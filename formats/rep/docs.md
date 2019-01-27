# .rep format

## Brief
.rep files store directions for ingame cutscenes. Most of the data is stored together with **timestamp**, which determines the start of data validness. 

For each animated object there is a stream of *transformations*, which include position, rotation and animation of given objects. 

In addition, camera positions and camera focus are tracked.

Eventually, the file contains the activation times of MafiaScript scripts and sound frames, and the beginnings of dialog speechs.

## Structure

1. File Header
2. List of animations
3. List of animated objects and their definitions
4. Stream of transformations
5. Stream of camera transformations
6. Stream of script/sound events
7. Dialogs

## 1. File header

| Offset  | Type | Name | Description
| ------------- | ------------- |  ------------- | ------------- |
| 0  | uint32_t | magicByte     | Identification string
| 4  | uint32_t  | sizeOfAnimationBlocks |  the size (in bytes) of all animation blocks together
| 8 | uint32_t |  sizeOfObjectDefinitionsSection  | the size (in bytes) of object definitions
| 12 | uint32_t | countOfObjectDefinitionBlocks |  Each object definition structure has 108 bytes
| 16 | uint32_t | fixedCCSequence | always CC CC CC CC
| 20 | uint32_t | countOfCameraChunks | each chunk has 64 bytes
| 24 | uint32_t | countOfCameraFocusChunks | each chunk has 56 bytes
| 28 | uint32_t | sizeOfScriptEventsSequence | together with sounds
| 32 | uint32_t | sizeOfDialogSection | unknown at the moment
| 36 | unsigned char | padding[60] | Filled with 00s
| 96 | uint32_t | countOfAnimationBlocks | the number of animation blocks following header
| 100 | uint32_t |  unknown |


## 2. List of animations
List of animations consist of **countOfAnimationBlocks** instances of AnimationBlock structure.

Each animation block assigns a unique ID to animation name, which is used in transformation stream afterwards.  
### AnimationBlock

Animation block has **52** bytes.

| Offset  | Type | Name | Description
| ------------- | ------------- |  ------------- | ------------- |
| 0  | uint32_t | animationID     | identifies animation and can be used to refer to animation in transformation stream
| 4  | char  | animationName[48] |  NULL-terminated string with suffix .i3d, the rest of section filled with CC 

## 3. List of object definitions

Object definition assigns actor name to scene's frame. It also defines the sizes of different transformation stream chunks. 

### AnimationObjectDefinition
| Offset  | Type | Name | Description
| ------------- | ------------- |  ------------- | ------------- |
| 0  | uint32_t | animationID     | identifies animation and can be used to refer to animation in transformation stream
| 4  | char  | animationName[48] |  NULL-terminated string with suffix .i3d, the rest of section filled with CC 