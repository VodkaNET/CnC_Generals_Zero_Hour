# Mobile ControlBar

Enable the preset in Options > Interface > Mobile UI. Both switches default to off.

Mobile Mode adds 30 percentage points to the stored Interface Scale and ControlBar
Scale independently: 100 becomes 130, 75 becomes 105, and 50 becomes 80. The sliders
retain their base values; the increase is applied at rendering time.

Mobile UI selects ControlBar.rml in this folder. It shares the desktop data model,
native commands, artwork, BLUE/DARK base styles, queue handling and promotions.
ControlBar.rcss here defines the mobile layout and is scaled with the HUD.
Changing the preset reloads the document without restarting the match.

The minimap sits at the top left. Minimap Size defaults to 100 and can be adjusted
from 50 to 150 independently of the HUD. Power and funds sit beside it. Promotions,
options, idle workers, beacon and diplomacy sit at the top right. Production and
the queue share the lower right edge. The Actions button sits at the middle left and opens three circular buttons to the right. Scatter, Stop, Attack Move and Cancel sit at the lower left.

Actions dispatch the engine's Select All, Select Matching Units and Create
Formation messages. Scatter, Stop and Attack Move use their existing meta commands.
Attack Move highlights while active and uses the next world click as its target.
Cancel clears placement or targeting first; otherwise it deselects the units.
Action controls are hidden for observers and replays.

This preset targets landscape displays. It expects the phone's input layer to
deliver mouse-style pointer events to the game. It does not implement multitouch,
gesture panning or a native Android/iOS renderer.

C++ syntax was checked with MSVC /Zs. No executable was built. Inspect the layout,
dropdown, radar and world targeting in the game after rebuilding z_generals.

The Icons folder contains the supplied selection, scatter, cancel and speed assets. Stop and Attack Move reuse SSStop and SSAttackMove2 from the game. Utility buttons keep their original proportions and sit below the visible promotion shortcuts.
