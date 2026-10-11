#pragma once

#include <RmlUi/Core/Element.h>
#include <memory>

class RmlModelPreviewElement : public Rml::Element
{
public:
    explicit RmlModelPreviewElement(const Rml::String &tag);
    ~RmlModelPreviewElement() override;
    void releaseModel();
protected:
    void OnRender() override;
    void OnUpdate() override;
private:
    struct State;
    std::unique_ptr<State> m_state;
    Rml::String m_object;
    double m_nextAttempt = 0;
};
