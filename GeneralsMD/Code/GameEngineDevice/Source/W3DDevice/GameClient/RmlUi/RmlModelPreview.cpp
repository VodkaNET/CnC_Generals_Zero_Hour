#include "W3DDevice/GameClient/RmlUi/RmlModelPreview.h"
#include "W3DDevice/GameClient/RmlUi/RmlUiRenderInterface.h"
#include "W3DDevice/GameClient/W3DDisplay.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "W3DDevice/GameClient/Module/W3DModelDraw.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/SystemInterface.h>
#include "WW3D2/camera.h"
#include "WW3D2/scene.h"
#include "WW3D2/light.h"
#include "WW3D2/ww3d.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/hanim.h"
#include <algorithm>
#include <cmath>
#include <cctype>

struct RmlModelPreviewElement::State
{
    SimpleSceneClass *scene = nullptr;
    CameraClass *camera = nullptr;
    RenderObjClass *model = nullptr;
    LightClass *light = nullptr;
    Vector3 center, extent;
    float radius = 1, angle = 0;
    double lastTime = 0;

    ~State()
    {
        if (scene && model) scene->Remove_Render_Object(model);
        if (scene && light) scene->Remove_Render_Object(light);
        if (model) model->Release_Ref();
        if (light) light->Release_Ref();
        if (camera) camera->Release_Ref();
        if (scene) scene->Release_Ref();
    }

    static void collectVisibleBounds(RenderObjClass *part, Vector3 &minimum, Vector3 &maximum, bool &found)
    {
        std::string name = part->Get_Name();
        for (char &c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (name.find("muzzle") != std::string::npos || name.find("flash") != std::string::npos ||
            part->Class_ID() == RenderObjClass::CLASSID_PARTICLEEMITTER || part->Class_ID() == RenderObjClass::CLASSID_PARTICLEBUFFER)
        {
            part->Set_Hidden(true);
            return;
        }
        if (part->Is_Hidden()) return;
        const int count = part->Get_Num_Sub_Objects();
        if (count > 0)
        {
            for (int i = 0; i < count; ++i)
                if (auto *child = part->Get_Sub_Object(i))
                {
                    collectVisibleBounds(child, minimum, maximum, found);
                    child->Release_Ref();
                }
            return;
        }
        const auto box = part->Get_Bounding_Box();
        const auto low = box.Center - box.Extent, high = box.Center + box.Extent;
        if (!found) { minimum = low; maximum = high; found = true; return; }
        minimum.X = (std::min)(minimum.X, low.X); minimum.Y = (std::min)(minimum.Y, low.Y); minimum.Z = (std::min)(minimum.Z, low.Z);
        maximum.X = (std::max)(maximum.X, high.X); maximum.Y = (std::max)(maximum.Y, high.Y); maximum.Z = (std::max)(maximum.Z, high.Z);
    }

    bool load(const Rml::String &name)
    {
        if (!TheThingFactory || !W3DDisplay::m_assetManager) return false;
        const auto *object = TheThingFactory->findTemplate(name.c_str(), FALSE);
        if (!object) return false;
        const auto &modules = object->getDrawModuleInfo();
        const ModelConditionInfo *condition = nullptr;
        for (Int i = 0; i < modules.getCount() && !condition; ++i)
        {
            const auto *module = modules.getNthData(i);
            const auto *draw = module ? module->getAsW3DModelDrawModuleData() : nullptr;
            if (!draw || !draw->m_attachToDrawableBone.isEmpty()) continue;
            condition = draw->findBestInfo(ModelConditionFlags());
            if (!condition && draw->m_defaultState >= 0 && draw->m_defaultState < Int(draw->m_conditionStates.size()))
                condition = &draw->m_conditionStates[draw->m_defaultState];
            if (!condition && !draw->m_conditionStates.empty()) condition = &draw->m_conditionStates.front();
            if (condition && (condition->m_modelName.isEmpty() || condition->m_modelName == "NONE" || condition->m_modelName == "None")) condition = nullptr;
        }
        if (!condition) return false;
        model = W3DDisplay::m_assetManager->Create_Render_Obj(condition->m_modelName.str());
        if (!model) return false;
        for (const auto &part : condition->m_hideShowVec)
            if (auto *sub = model->Get_Sub_Object_By_Name(part.subObjName.str()))
            {
                sub->Set_Hidden(part.hide);
                sub->Release_Ref();
            }
        for (const auto &idle : condition->m_animations)
            if (idle.isIdleAnim())
            {
                if (auto *animation = idle.getAnimHandle())
                {
                    model->Set_Animation(animation, 0, RenderObjClass::ANIM_MODE_LOOP);
                    animation->Release_Ref();
                }
                break;
            }
        const auto bounds = model->Get_Bounding_Box();
        center = bounds.Center;
        extent = bounds.Extent;
        Vector3 minimum, maximum;
        bool found = false;
        collectVisibleBounds(model, minimum, maximum, found);
        if (found)
        {
            center = (minimum + maximum) * 0.5f;
            extent = (maximum - minimum) * 0.5f;
        }
        radius = (std::max)(extent.Length(), 1.0f);
        scene = new SimpleSceneClass();
        scene->Set_Ambient_Light(Vector3(0.5f, 0.5f, 0.55f));
        scene->Set_Fog_Enable(false);
        scene->Add_Render_Object(model);
        camera = new CameraClass();
        camera->Set_Clip_Planes(radius * 0.05f, radius * 30.0f);
        Matrix3D view(true);
        view.Look_At(Vector3(radius * 2.8f, -radius * 4.2f, radius * 2.2f), Vector3(0, 0, 0), 0);
        camera->Set_Transform(view);
        light = new LightClass(LightClass::DIRECTIONAL);
        light->Set_Diffuse(Vector3(0.9f, 0.9f, 0.85f));
        Matrix3D lighting(true);
        lighting.Look_At(Vector3(-1, -2, 3), Vector3(0, 0, 0), 0);
        light->Set_Transform(lighting);
        scene->Add_Render_Object(light);
        lastTime = Rml::GetSystemInterface()->GetElapsedTime();
        return true;
    }

    void render(Rml::Vector2f position, Rml::Vector2f size, Rml::Vector2i screen)
    {
        if (!WW3D::Is_Rendering() || screen.x <= 0 || screen.y <= 0) return;
        const double now = Rml::GetSystemInterface()->GetElapsedTime();
        angle = std::fmod(angle + float(std::clamp(now - lastTime, 0.0, 0.1)) * 0.5f, 6.2831853f);
        lastTime = now;
        Matrix3D rotation(true);
        rotation.Rotate_Z(angle);
        rotation.Translate(-center);
        model->Set_Transform(rotation);
        Matrix3D view(true);
        view.Look_At(Vector3(2.8f, -4.2f, 2.2f), Vector3(0, 0, 0), 0);
        const float horizontal = std::sqrt(extent.X * extent.X + extent.Y * extent.Y);
        const auto projected = [horizontal, this](const Vector3 &axis) {
            return horizontal * std::sqrt(axis.X * axis.X + axis.Y * axis.Y) + extent.Z * std::abs(axis.Z);
        };
        const float aspect = size.x / size.y;
        const float halfWidth = (std::max)(projected(view.Get_X_Vector()), projected(view.Get_Y_Vector()) * aspect) / 0.9f;
        const float halfHeight = halfWidth / aspect;
        const float distance = radius * 3.0f;
        camera->Set_Projection_Type(CameraClass::ORTHO);
        camera->Set_View_Plane(Vector2(-halfWidth, -halfHeight), Vector2(halfWidth, halfHeight));
        view.Set_Translation(view.Get_Z_Vector() * distance);
        camera->Set_Transform(view);
        camera->Set_Clip_Planes(radius * 0.01f, distance + radius * 4.0f);
        camera->Set_Viewport(Vector2(std::clamp(position.x / screen.x, 0.0f, 1.0f), std::clamp(position.y / screen.y, 0.0f, 1.0f)),
            Vector2(std::clamp((position.x + size.x) / screen.x, 0.0f, 1.0f), std::clamp((position.y + size.y) / screen.y, 0.0f, 1.0f)));
        auto *device = DX8Wrapper::_Get_D3D_Device8();
        D3DVIEWPORT8 viewport;
        device->GetViewport(&viewport);
        WW3D::Render(scene, camera, false, true);
        DX8Wrapper::Set_Viewport(&viewport);
    }
};

RmlModelPreviewElement::RmlModelPreviewElement(const Rml::String &tag) : Rml::Element(tag) {}
RmlModelPreviewElement::~RmlModelPreviewElement() = default;

void RmlModelPreviewElement::releaseModel()
{
    m_state.reset();
    m_object.clear();
    m_nextAttempt = 0;
}

void RmlModelPreviewElement::OnUpdate()
{
    Rml::Element::OnUpdate();
    if (!IsVisible(true)) { releaseModel(); return; }
}

void RmlModelPreviewElement::OnRender()
{
    Rml::Element::OnRender();
    const auto object = GetAttribute<Rml::String>("object", "");
    if (object.empty()) return;
    if (object != m_object) { releaseModel(); m_object = object; }
    const double now = Rml::GetSystemInterface()->GetElapsedTime();
    if (!m_state && now >= m_nextAttempt)
    {
        auto state = std::make_unique<State>();
        if (state->load(object)) m_state = std::move(state);
        else m_nextAttempt = now + 2.0;
    }
    if (!m_state) return;
    const auto size = GetBox().GetSize(Rml::BoxArea::Content);
    if (size.x <= 0 || size.y <= 0) return;
    struct Draw
    {
        State *state;
        Rml::Vector2f position, size;
        Rml::Vector2i screen;
    } draw = {m_state.get(), GetAbsoluteOffset(Rml::BoxArea::Content), size, GetContext()->GetDimensions()};
    auto *renderer = static_cast<RmlUiRenderInterface *>(Rml::GetRenderInterface());
    if (renderer) renderer->drawEngine([](void *context) {
        const auto &draw = *static_cast<const Draw *>(context);
        draw.state->render(draw.position, draw.size, draw.screen);
    }, &draw);
}
