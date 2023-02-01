#include "raylib.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "raymath.h"
#include "rlgl.h"
#include <float.h>

#define DEV_MODE // comment out to remove dev text

// TODO continue work with corridors
// make corridors work with all iterations of the map generation
// (currently only works with the last iteration)
// split up the large one line equations to multiple smaller lines for improved readability
const int screenHeight = 1080;
const int screenWidth = 1920;
const int targetFPS = 60;
const float mapSectionMarginPercentage = 0.3; // how many % magin from other map sections (1 = 100%)
const int iterations = 2;                     // iterations for the generator
const int xySplitRandomizer = 12;
int xySplitRandomizerThreshold = xySplitRandomizer / 2; // will get bigger/smaller depending on the previous split, this is used to avoid too many x/y slices happening after one another
const float roomMarginPercentage = 0.1;                 // 1% percision, will be rounded afterwards
const float roomMinSizePercentage = 0.5;                // 1% percision, will be rounded afterwards
const float constCorridorWidth = 4;                     // set size for corridors
const float minCorridorWidth = 3;                       // min size tolerance for valid intersects

typedef struct rect_t
{
    Vector2 startPos;
    Vector2 endPos;
} rect_t;

typedef struct mapSection_t
{
    rect_t area;
    rect_t room;
    rect_t corridor;
    struct mapSection_t *splitMapSections;
} mapSection_t;

// Recursive function to get every room in every iteration of the randomly generated map
void GetRoom(int currentIteration, int *roomsCount, rect_t *rooms, mapSection_t mapSection)
{

    if (currentIteration <= iterations)
    {
        GetRoom(currentIteration + 1, roomsCount, rooms, mapSection.splitMapSections[0]);
        GetRoom(currentIteration + 1, roomsCount, rooms, mapSection.splitMapSections[1]);
    }
    else
    {
        rooms[*roomsCount] = mapSection.room;
        *roomsCount = *roomsCount + 1;
    }
}

void GenerateGridRooms(rect_t room)
{
    
}

void FreeBSPMap(int iterationCount, int desiredIterations, mapSection_t *mapSection)
{
    if (iterationCount <= desiredIterations)
    {
        iterationCount++;
        FreeBSPMap(iterationCount, desiredIterations, (mapSection->splitMapSections));
        FreeBSPMap(iterationCount, desiredIterations, (mapSection->splitMapSections + 1));
        free(mapSection->splitMapSections);
    }
    else
    {
        free(mapSection->splitMapSections);
    }
}

Vector2 GetRectSize(rect_t rect)
{
    return (Vector2){rect.endPos.x - rect.startPos.x, rect.endPos.y - rect.startPos.y};
}

void DrawCorridor(rect_t corridor)
{
    Vector2 size = GetRectSize(corridor);
    DrawRectangleV(corridor.startPos, size, PINK);
#ifdef DEV_MODE
    // DrawText(TextFormat("size: x:%f, y:%f", size.x, size.y), corridor.startPos.x, corridor.startPos.y, 20, PURPLE);
#endif
}

void DrawRoom(rect_t room)
{
    Vector2 size = GetRectSize(room);
    // puts("drawing room");
    float textPosX = room.startPos.x + ((room.endPos.x - room.startPos.x) / 2);
    float textPosY = room.startPos.y + ((room.endPos.y - room.startPos.y) / 2);
    DrawRectangleV(room.startPos, size, PURPLE);
#ifdef DEV_MODE
    // DrawText(TextFormat("start pos: %.2f, %.2f\n"
    //                     "end pos:   %.2f, %.2f\n",
    //                     room.startPos.x, room.startPos.y,
    //                     room.endPos.x, room.endPos.y),
    //          textPosX, textPosY, 1, GREEN);
#endif
}

void DrawBSPMapSections(int iterationCount, int desiredIterations, mapSection_t mapSection, Color *color, int colorIndex)
{
#ifdef DEV_MODE
    DrawLineV(mapSection.splitMapSections[0].area.endPos, mapSection.splitMapSections[1].area.startPos, color[colorIndex]);
#endif
    if (iterationCount >= desiredIterations)
    {
        DrawRoom(mapSection.splitMapSections[0].room);
        DrawRoom(mapSection.splitMapSections[1].room);
    }
    else
    {

        iterationCount++;
        colorIndex++;
        DrawBSPMapSections(iterationCount, desiredIterations, mapSection.splitMapSections[0], color, colorIndex);

        DrawBSPMapSections(iterationCount, desiredIterations, mapSection.splitMapSections[1], color, colorIndex);
    }
    DrawCorridor(mapSection.corridor);
#ifdef DEV_MODE
    // DrawText(TextFormat("%.2f, %.2f", mapSection.splitMapSections[0].area.endPos.x, mapSection.splitMapSections[0].area.endPos.y),
    //          mapSection.splitMapSections[0].area.endPos.x,
    //          mapSection.splitMapSections[0].area.endPos.y, 10, LIME);
    // DrawText(TextFormat("%.2f, %.2f", mapSection.splitMapSections[1].area.startPos.x, mapSection.splitMapSections[1].area.startPos.y),
    //          mapSection.splitMapSections[1].area.startPos.x,
    //          mapSection.splitMapSections[1].area.startPos.y, 10, LIME);
#endif
}
// early idea of getting valid intersects for bigger map section corridors
void GetIntersect(rect_t *validIntersects, int *validIntersectsPointer, rect_t room1, rect_t room2, bool isSlicedOnXAxis)
{
    if (isSlicedOnXAxis)
    {
        // if intersect is valid
        if (room1.startPos.y > room2.endPos.y - minCorridorWidth ||
            room2.startPos.y > room1.endPos.y - minCorridorWidth)
        {
            return;
        }
        float intersectStart = room1.startPos.y > room2.startPos.y ? room1.startPos.y : room2.startPos.y;
        float intersectEnd = room1.endPos.y < room2.endPos.y ? room1.endPos.y : room2.endPos.y;
        // the intersect is in its respective cordinate, the distance between rooms is in the other cordinate
        rect_t validIntersect = (rect_t){
            (Vector2){room1.endPos.x, intersectStart},
            (Vector2){room2.startPos.x, intersectEnd}};
        // the idea is to get a array of valid intersects, pick one by random and then get a random position inside that valid intersect
        validIntersects[*validIntersectsPointer] = validIntersect;
        *validIntersectsPointer = *validIntersectsPointer + 1;
    }
    else
    {
        if (room1.startPos.x > room2.endPos.x - minCorridorWidth ||
            room2.startPos.x > room1.endPos.x - minCorridorWidth)
        {
            return;
        }
        float intersectStart = room1.startPos.x > room2.startPos.x ? room1.startPos.x : room2.startPos.x;
        float intersectEnd = room1.endPos.x < room2.endPos.x ? room1.endPos.x : room2.endPos.x;
        rect_t validIntersect = (rect_t){
            (Vector2){intersectStart, room1.endPos.y},
            (Vector2){intersectEnd, room2.startPos.y}};
        validIntersects[*validIntersectsPointer] = validIntersect;
        *validIntersectsPointer += 1;
    }
    printf("intersect pointer inside getIntersect: %d\n", *validIntersectsPointer);
}

void FindValidCorridorTargets(int currentIteration, mapSection_t mapSection, rect_t *targets, int *targetsPointer, bool isSplitOnXAxis, bool isLeftOrAbove)
{
    puts("wee 0");
    printf("current iteration: %d\n", currentIteration);
    bool currentIsSplitOnXAxis = mapSection.splitMapSections[0].area.startPos.y == mapSection.splitMapSections[1].area.startPos.y;
    puts("wee 1");
    if (currentIteration < iterations - 1)
    {
        puts("wee 2");
        // checks if they are split on the same axis
        if (currentIsSplitOnXAxis != isSplitOnXAxis)
        {
            currentIteration++;
            // targets[*targetsPointer] = mapSection.corridor;
            //*targetsPointer += 1;
            FindValidCorridorTargets(currentIteration, mapSection.splitMapSections[0], targets, targetsPointer, isSplitOnXAxis, isLeftOrAbove);
            FindValidCorridorTargets(currentIteration, mapSection.splitMapSections[1], targets, targetsPointer, isSplitOnXAxis, isLeftOrAbove);
        }
        else
        {
            currentIteration++;
            // if isLeftOrAbove is true then the adjacent room is 0, otherwise its 1
            FindValidCorridorTargets(currentIteration, mapSection.splitMapSections[isLeftOrAbove], targets, targetsPointer, isSplitOnXAxis, isLeftOrAbove);
        }
        puts("wee 3");
    }
    else
    {
        puts("wee 4");
        if (currentIsSplitOnXAxis != isSplitOnXAxis)
        {
            targets[*targetsPointer] = mapSection.corridor;
            *targetsPointer += 1;
            targets[*targetsPointer] = mapSection.splitMapSections[0].room;
            *targetsPointer += 1;
            targets[*targetsPointer] = mapSection.splitMapSections[1].room;
            *targetsPointer += 1;
        }
        else
        {
            targets[*targetsPointer] = mapSection.splitMapSections[isLeftOrAbove].room;
            *targetsPointer += 1;
        }
        puts("wee 5");
    }
}

void GenerateCorridor(bool isSlicedOnXAxis, mapSection_t *mapSection, rect_t randIntersect)
{
#ifdef DEV_MODE
    printf("is sliced on x axis: %d\n",
           isSlicedOnXAxis);
#endif
    puts("waa 1");
    int intersectStart = 0;
    int intersectEnd = 0;
    if (isSlicedOnXAxis)
    {
        intersectStart = randIntersect.startPos.y;
        intersectEnd = randIntersect.endPos.y;
    }
    else
    {
        intersectStart = randIntersect.startPos.x;
        intersectEnd = randIntersect.endPos.x;
    }
    puts("waa 2");
    float intersectWidth = intersectEnd - intersectStart;
    float randPercent = ((float)(rand() % 100) / 100);
    // if the start posision + corridor width would go outside the intersecting bonds, subtract the corridor width from total
    float corridorStartPoint;
    float corridorWidth = constCorridorWidth;
    // if intersection width is smaller than corridor width, reduce corridor width
    if (intersectWidth <= constCorridorWidth)
    {
        corridorStartPoint = intersectStart;
        corridorWidth -= (constCorridorWidth - intersectWidth);
    }
    // if the corridor would go outside intersecting bounds
    else if (randPercent * intersectWidth >= intersectWidth - corridorWidth)
    {
        corridorStartPoint = intersectStart + intersectWidth - corridorWidth;
    }
    else
    {
        corridorStartPoint = intersectStart + (intersectWidth * randPercent);
    }
    puts("waa 3");
    if (isSlicedOnXAxis)
    {
        mapSection->corridor.startPos = (Vector2){randIntersect.startPos.x, corridorStartPoint};
        mapSection->corridor.endPos = (Vector2){randIntersect.endPos.x, corridorStartPoint + corridorWidth};

#ifdef DEV_MODE
        printf("rand intersect end pos x: %f, rand intersect start pos x: %f\n", randIntersect.startPos.x, randIntersect.endPos.x);
#endif
    }
    else
    {
        mapSection->corridor.startPos = (Vector2){corridorStartPoint, randIntersect.startPos.y};
        mapSection->corridor.endPos = (Vector2){corridorStartPoint + corridorWidth, randIntersect.endPos.y};
    }
    puts("waa 4");
}

void RoomToTile()
{
    // printf("value of xwidth 1: %g", xWidth);
}

void GenerateRoom(rect_t *room, rect_t area)
{
#ifdef DEV_MODE
    puts("generating room");
#endif
    float xWidth = area.endPos.x - area.startPos.x;
    float yWidth = area.endPos.y - area.startPos.y;
    // Making pointers of Y and X
    // float *xWidthPointer;
    // float *yWidthPointer;
    // xWidthPointer = &xWidth;
    // yWidthPointer = &yWidth;

    Vector2 startPos = {
        // base value is beginning of the section + margin, then adds a random sum between 0 and the map section width - margin and min size (in order to make sure there will always be a min size available)
        area.startPos.x + (xWidth * roomMarginPercentage) + xWidth * ((float)(rand() % (int)(100 - (2 * roomMarginPercentage * 100) - (roomMinSizePercentage * 100))) / 100),
        area.startPos.y + (yWidth * roomMarginPercentage) + yWidth * ((float)(rand() % (int)(100 - (2 * roomMarginPercentage * 100) - (roomMinSizePercentage * 100))) / 100)};

    // mod 0 does not execute, eg you would get the entire rand() value
    int xRandExtraSizeMaxPercentage = 100 - (roomMinSizePercentage * 100) - (roomMarginPercentage * 100) - (float)(((startPos.x - area.startPos.x) / xWidth) * 100);
    float xEndPosExtraSize = xRandExtraSizeMaxPercentage > 0 ? xWidth * (float)((rand() % xRandExtraSizeMaxPercentage) / 100) : 0;

    int yRandExtraSizeMaxPercentage = 100 - (roomMinSizePercentage * 100) - (roomMarginPercentage * 100) - (float)(((startPos.y - area.startPos.y) / yWidth) * 100);
    float yEndPosExtraSize = yRandExtraSizeMaxPercentage > 0 ? yWidth * (float)((rand() % yRandExtraSizeMaxPercentage) / 100) : 0;

    Vector2 endPos = {
        // base value is map section width * min size, then adds a random value between 0 and the distance from the startpos of the room to the end of the map section - margin
        startPos.x + (xWidth * roomMinSizePercentage) + xEndPosExtraSize,
        startPos.y + (yWidth * roomMinSizePercentage) + yEndPosExtraSize};
#ifdef DEV_MODE
    printf("map section start pos: %.2f, %.2f\n"
           "map section end pos:   %.2f, %.2f\n"
           "map section width:     %.2f, %.2f\n"
           "test:                  %.17f\n"
           "test 2:                %.17f\n"
           "room: \n"
           "start pos: %.2f, %.2f\n"
           "end pos:   %.2f, %.2f\n",
           area.startPos.x, area.startPos.y,
           area.endPos.x, area.endPos.y,
           xWidth, yWidth,
           ((float)(int)(100 - (roomMinSizePercentage * 100) - (roomMarginPercentage * 100) - (float)(((startPos.x - area.startPos.x) / xWidth) * 100))),
           (float)(((startPos.x - area.startPos.x) / xWidth) * 100),
           startPos.x, startPos.y,
           endPos.x, endPos.y);
#endif
    *room = (rect_t){
        .startPos = startPos,
        .endPos = endPos};
}

void GenerateBSPMapSections(int iterationCount, int desiredIterations, mapSection_t *mapSection)
{
    mapSection->splitMapSections = (mapSection_t *)calloc(2, sizeof(mapSection_t));
#ifdef DEV_MODE
    printf("gen iteration: %d\n", iterationCount);
#endif

    if (rand() % xySplitRandomizer >= xySplitRandomizerThreshold)
    {
        xySplitRandomizerThreshold++;
#ifdef DEV_MODE
        puts("split in x");
#endif
        // splits in random place with atleast a 20% margin to the top/bottom
        float xWidth = mapSection->area.endPos.x - mapSection->area.startPos.x;
        float xSplit = (((rand() % (int)(xWidth * (1 - (mapSectionMarginPercentage * 2)))) * 100) / 100) + mapSection->area.startPos.x + (xWidth * mapSectionMarginPercentage);
        // float xSplit = (((rand() % (int)((xWidth) * (1 - (0.8)))) * 100) / 100) + mapSection->area.startPos.x + ((xWidth) * 0.4);
        //  float xSplit = (mapSection->area.startPos.x + mapSection->area.endPos.x) / 2;
        mapSection->splitMapSections[0] = (mapSection_t){
            .area.startPos = (Vector2){mapSection->area.startPos.x, mapSection->area.startPos.y},
            .area.endPos = (Vector2){xSplit, mapSection->area.endPos.y}};

        mapSection->splitMapSections[1] = (mapSection_t){
            .area.startPos = (Vector2){xSplit, mapSection->area.startPos.y},
            .area.endPos = (Vector2){mapSection->area.endPos.x, mapSection->area.endPos.y}};
    }
    else
    {
        xySplitRandomizerThreshold--;
#ifdef DEV_MODE
        puts("split in y");
#endif
        // splits in random place with atleast a mapSectionMarginPercentage margin to the left/right
        float yWidth = mapSection->area.endPos.y - mapSection->area.startPos.y;
        float ySplit = (((rand() % (int)(yWidth * (1 - (mapSectionMarginPercentage * 2)))) * 100) / 100) + mapSection->area.startPos.y + (yWidth * mapSectionMarginPercentage);
        // float ySplit = (((rand() % (int)((yWidth) * (1 - (0.8)))) * 100) / 100) + mapSection->area.startPos.y + ((yWidth) * 0.4);

        // float ySplit = (mapSection->area.startPos.y + mapSection->area.endPos.y) / 2;
        mapSection->splitMapSections[0] = (mapSection_t){
            .area.startPos = (Vector2){mapSection->area.startPos.x, mapSection->area.startPos.y},
            .area.endPos = (Vector2){mapSection->area.endPos.x, ySplit}};

        mapSection->splitMapSections[1] = (mapSection_t){
            .area.startPos = (Vector2){mapSection->area.startPos.x, ySplit},
            .area.endPos = (Vector2){mapSection->area.endPos.x, mapSection->area.endPos.y}};
    }
    if (iterationCount >= desiredIterations)
    {
#ifdef DEV_MODE
        printf("all iterations complete: iterations: %d, desired iterations: %d\n",
               iterationCount, desiredIterations);

#endif
        GenerateRoom(&mapSection->splitMapSections[0].room, mapSection->splitMapSections[0].area);
        GenerateRoom(&mapSection->splitMapSections[1].room, mapSection->splitMapSections[1].area);
        bool isSplitOnXAxis = mapSection->splitMapSections[0].area.startPos.y == mapSection->splitMapSections[1].area.startPos.y;
        rect_t intersect[100]; // = {(Vector2){0, 0}, (Vector2){0, 0}};
        int temp = 0;
        GetIntersect(intersect, &temp, mapSection->splitMapSections[0].room, mapSection->splitMapSections[1].room, isSplitOnXAxis);
        GenerateCorridor(isSplitOnXAxis, mapSection, intersect[0]);
        // return;
    }
    else
    {
        iterationCount++;
        GenerateBSPMapSections(iterationCount, desiredIterations, (mapSection->splitMapSections));
        GenerateBSPMapSections(iterationCount, desiredIterations, (mapSection->splitMapSections + 1));
        iterationCount--;
        puts("woo 1");
        bool isSplitOnXAxis = mapSection->splitMapSections[0].area.startPos.y == mapSection->splitMapSections[1].area.startPos.y;
        puts("woo 2");
        rect_t leftOrAboveValidCorridorTargets[100];
        int leftOrAboveValidCorridorTargetsPointer = 0;
        rect_t rightOrBelowValidCorridorTargets[100];
        int rightOrBelowValidCorridorTargetsPointer = 0;
        puts("woo 3");
        FindValidCorridorTargets(iterationCount, mapSection->splitMapSections[0], leftOrAboveValidCorridorTargets, &leftOrAboveValidCorridorTargetsPointer, isSplitOnXAxis, true);
        puts("woo 3,5");
        FindValidCorridorTargets(iterationCount, mapSection->splitMapSections[1], rightOrBelowValidCorridorTargets, &rightOrBelowValidCorridorTargetsPointer, isSplitOnXAxis, false);
        puts("woo 4");
        rect_t validIntersects[200];
        int validIntersectsPointer = 0;
        puts("woo 5");
        for (int l = 0; l < leftOrAboveValidCorridorTargetsPointer; l++)
        {
            for (int r = 0; r < rightOrBelowValidCorridorTargetsPointer; r++)
            {
                GetIntersect(validIntersects, &validIntersectsPointer, leftOrAboveValidCorridorTargets[l], rightOrBelowValidCorridorTargets[r], isSplitOnXAxis);
            }
        }
        puts("woo 6");
        printf("intersect pointer: %d\n", validIntersectsPointer);
        int randIntersectTarget = rand() % validIntersectsPointer + 1;
        puts("woo 6.5");
        GenerateCorridor(isSplitOnXAxis, mapSection, validIntersects[0]);
        puts("woo 7");
    }
}

int main()
{
    srand(time(NULL));
    InitWindow(screenWidth, screenHeight, "map generation test");
    SetTargetFPS(targetFPS);

    mapSection_t map = (mapSection_t){
        .area.startPos = (Vector2){0, 0},
        .area.endPos = (Vector2){64, 64},
        .splitMapSections = NULL};

    GenerateBSPMapSections(0, iterations, &map);

    Image image = LoadImage("textures/MainCharacterIdle.png");    // Loading in image for main character texture
    Texture2D textureMaincharacter = LoadTextureFromImage(image); // Make image into texture
    UnloadImage(image);                                           // Unload image from CPU memory

    Image floor1 = LoadImage("textures/wall1Tile.png");     // Loading in image for main character texture
    Texture2D textureFloor1 = LoadTextureFromImage(floor1); // Make image into texture
    UnloadImage(floor1);                                    // Unload image from CPU memory

    float textureposX = screenWidth / 2 - textureMaincharacter.width / 2; // Floats for the main characters position
    float textureposY = screenHeight / 2 - textureMaincharacter.height / 2;

    Camera2D characterCamera = {0};
    characterCamera.target = (Vector2){textureposX + 20.0f, textureposY + 20.0f};
    characterCamera.offset = (Vector2){screenWidth / 1.8f, screenHeight / 1.7f};
    characterCamera.zoom = 5;

    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_W)) // Keyboard controls for main character
        {
            textureposY -= 4.0f;
        }
        if (IsKeyDown(KEY_A))
        {
            textureposX -= 4.0f;
        }
        if (IsKeyDown(KEY_S))
        {
            textureposY += 4.0f;
        }
        if (IsKeyDown(KEY_D))
        {
            textureposX += 4.0f;
        }
        characterCamera.target = (Vector2){textureposX + 8, textureposY + 8};

        characterCamera.zoom += ((float)GetMouseWheelMove() * 0.05f); // Character zoom with scrollwheel

        if (IsKeyPressed(KEY_F))
        {
            ToggleFullscreen();
        }
        if (IsKeyPressed(KEY_SPACE))
        {
            FreeBSPMap(0, iterations, &map);
            GenerateBSPMapSections(0, iterations, &map);
        }

        Color colors[] = {RED, YELLOW, GREEN, BLUE};

        BeginDrawing();
        ClearBackground(BLACK);
        // CameraController
        BeginMode2D(characterCamera);
        {
            DrawBSPMapSections(0, iterations, map, colors, 0);
            DrawTexture(textureMaincharacter, textureposX, textureposY, WHITE);
            DrawLine((int)characterCamera.target.x, -screenHeight * 10, (int)characterCamera.target.x, screenHeight * 10, GREEN);
            DrawLine(-screenWidth * 10, (int)characterCamera.target.y, screenWidth * 10, (int)characterCamera.target.y, GREEN);
        }
        EndMode2D();
        EndDrawing();
    }

    UnloadTexture(textureMaincharacter); // Unload texture from GPU memory

    CloseWindow();
    return 0;
}