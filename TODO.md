Tasks for Claude:

# Incomplete Tasks

* [X] BlastEm updates
    * [X] If I don't specify a file extenstion, put a default extension of .sprec on sprite recordings
    * [X] Do a code review of the sprite recording logic, particularly if a sprite isn't DMA-ed from ROM, have we correctly copied the sprite and palette data from VRAM into the sprec file

* [X] Sprite Editor
    * [X] Sprite Viewer
        * [X] Sprites captured in RAM indicate they are from ROM but at the RAM address, change the text to say ROM as well
    * [X] Sprite Editor tab
        * [X] Remove the palette at the bottom of the screen since we have one on the right side
        * [X] Want middle click to select a sprite in the sprite group (would be important if sprites in the group had different palettes)
        * [X] When a sprite from a group is middle clicked, display sprite info (ROM address, dimensions, palette addr)
        * [X] Provide a button to delete the selected sprite from the sprite group (like accidentally capturing sprite from a neighboring character)

* [ ]  BN-Genesis Plugin

    * [X] Load a game definition file when a ROM is loaded and label all sprites at their ROM addresses
    * [X] Give each sprite a proper structure at its memory location (you will need to define a structure for sprite data and make sure it is loaded into the bndb file)
    * [X] Label each palette in the ROM with an appropriate structure
    * [ ] Show a sprite in a custom Binja pane with palette selection (future task - don't do this one yet)
    * [X] Write a small document about how we can get code flow info from blastem into binary ninja

# Completed Tasks

