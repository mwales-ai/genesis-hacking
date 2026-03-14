Tasks for Claude:

# Incomplete Tasks

## Sprite Editor

* [X] Painting tools
    * [X] Make a palette tool that is 4x4 square of the colors
    * [X] The pencil, full, eyedropper, and brush tools should be buttons with icons on them, not words.  Make icon buttons 2 x 2 buttons.  Size for brush should appear under the tool buttons and hide depending on brush icon active.
* [X] Screen Capture
    * [X] Need a save changes to ROM button, and a revert to original ROM button to discard painting done so far
    * [X] Add painting tools to screen capture when you press the edit button
* [ ] Sprite Viewer Improvments
    * [ ] The right pane canvas isn't big enough to view multiple sprites. When a sprite group is clicked, the whole group should be visible
    * [X] When sprites are captures from an animation, they don't show up in the viewer until I save the JSON and reopen the tool
    * [X] Add a delete button so we can delete a sprite group from JSON file
    * [X] When you double click the sprite in the right panel and you got to the raw tile browser, the W and H fields are changed, but need
          the actual W and H setting to change in the viewer (it was still displaying sprites as 1x1

## BN-Genesis Plugin (Future — do not work on yet)

* [ ] Load a game definition file when a ROM is loaded and label all sprites at their ROM addresses
* [ ] Give each sprite a proper structure at its memory location
* [ ] Label each palette in the ROM with an appropriate structure
* [ ] Show a sprite in a custom Binja pane with palette selection

# Completed Tasks

