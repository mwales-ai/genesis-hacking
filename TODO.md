Tasks for Claude:

# Incomplete Tasks

* [ ] Sprite Viewer
    * [ ] Right pane border.  Don't have the borders get thicker when zoomed in
* [ ] Screen Capture tool
    * [ ] Bottom of screen needs some status about capture.  Number of unique patterns store, number of the patterns that have ROM address recovered.  Number of palettes.  Are the pallete ROM addresses recovered?
    * [ ] When mousing over a pattern tile, dispaly the address information for the tile (memory address, palatte address
    * [ ] Saving changes to ROM didn't work when I tested it, but I don't know if that is because the pattern info had valid address information or not
* [ ] Whenever any save to the ROM occurs, if we are writing to the original ROM file, prompt them to change the name of their custom ROM so they don't overwrite the original ROM.
* [ ] Use Aladdin_Beta.bin and aladdin_sprite_def.json, and ali_animations.sprec as examples to update the documentation for the sprite editor (Do this step last so the documentation gets all the other changes incorporated)
* [ ] Add a screenshot of the spirte editor to the main project readme page so people can see how awesome this project is


## Sprite Editor

* [X] Painting tools
    * [X] Make a palette tool that is 4x4 square of the colors
    * [X] The pencil, full, eyedropper, and brush tools should be buttons with icons on them, not words.  Make icon buttons 2 x 2 buttons.  Size for brush should appear under the tool buttons and hide depending on brush icon active.
* [X] Screen Capture
    * [X] Need a save changes to ROM button, and a revert to original ROM button to discard painting done so far
    * [X] Add painting tools to screen capture when you press the edit button
* [ ] Sprite Viewer Improvments
    * [X] The right pane canvas isn't big enough to view multiple sprites. When a sprite group is clicked, the whole group should be visible
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

