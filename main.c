#include "raylib.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
// TODO make mapSectionMargin work to modify room sizes, add rooms and connect them
const int screenHeight = 720;
const int screenWidth = 1280;
const int targetFPS = 60;
const float mapSectionMarginPercentage = 0.4; // how many % magin from other map sections (1 = 100%)
const int iterations = 3;                     // iterations for the generator
const int xySplitRandomizer = 8;
int xySplitRandomizerThreshold = xySplitRandomizer / 2; // will get bigger/smaller depending on the previous split, this is used to avoid too many x/y slices happening after one another
const float roomMarginPercentage = 0.1;                 // 1% percision, will be rounded afterwards
const float roomMinSizePercentage = 0.5;                // 1% percision, will be rounded afterwards

typedef struct rect_t
{
    Vector2 startPos;
    Vector2 endPos;
} rect_t;

typedef struct mapSection_t
{
    rect_t area;
    rect_t room;
    rect_t splitMapSecionsCorridor;
    struct mapSection_t *splitMapSections;
} mapSection_t;

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

void DrawRoom(rect_t room)
{
    Vector2 size = GetRectSize(room);
    // puts("drawing room");
    float textPosX = room.startPos.x + ((room.endPos.x - room.startPos.x) / 2);
    float textPosY = room.startPos.y + ((room.endPos.y - room.startPos.y) / 2);
    DrawRectangleV(room.startPos, size, BLUE);
    DrawText(TextFormat("start pos: %.2f, %.2f\n"
                        "end pos:   %.2f, %.2f\n",
                        room.startPos.x, room.startPos.y,
                        room.endPos.x, room.endPos.y),
             textPosX, textPosY, 20, GREEN);
}

void DrawBSPMapSections(int iterationCount, int desiredIterations, mapSection_t mapSection, Color color)
{

    if (iterationCount >= desiredIterations)
    {
        DrawRoom(mapSection.splitMapSections[0].room);
        DrawRoom(mapSection.splitMapSections[1].room);
    }
    else
    {
        iterationCount++;

        DrawBSPMapSections(iterationCount, desiredIterations, mapSection.splitMapSections[0], color);

        DrawBSPMapSections(iterationCount, desiredIterations, mapSection.splitMapSections[1], color);
    }
    DrawLineV(mapSection.splitMapSections[0].area.endPos, mapSection.splitMapSections[1].area.startPos, color);
    DrawText(TextFormat("%.2f, %.2f", mapSection.splitMapSections[0].area.endPos.x, mapSection.splitMapSections[0].area.endPos.y),
             mapSection.splitMapSections[0].area.endPos.x,
             mapSection.splitMapSections[0].area.endPos.y, 10, LIME);
    DrawText(TextFormat("%.2f, %.2f", mapSection.splitMapSections[1].area.startPos.x, mapSection.splitMapSections[1].area.startPos.y),
             mapSection.splitMapSections[1].area.startPos.x,
             mapSection.splitMapSections[1].area.startPos.y, 10, LIME);
}

void GenerateRoom(rect_t *room, rect_t area)
{
    puts("generating room");
    float xWidth = area.endPos.x - area.startPos.x;
    float yWidth = area.endPos.y - area.startPos.y;
    Vector2 startPos = {
        // base value is beginning of the section + margin, then adds a random sum between 0 and the map section width - margin and min size (in order to make sure there will always be a min size available)
        area.startPos.x + (xWidth * roomMarginPercentage) + xWidth * ((float)(rand() % (int)(100 - (2 * roomMarginPercentage * 100) - (roomMinSizePercentage * 100))) / 100),
        area.startPos.y + (yWidth * roomMarginPercentage) + yWidth * ((float)(rand() % (int)(100 - (2 * roomMarginPercentage * 100) - (roomMinSizePercentage * 100))) / 100)};

        int xRandExtraSizeMaxPercentage = 100 - (roomMinSizePercentage * 100) - (roomMarginPercentage * 100) - (float)(((startPos.x - area.startPos.x) / xWidth) * 100);
        float xEndPosExtraSize = xRandExtraSizeMaxPercentage > 0 ? xWidth * (float)((rand() % xRandExtraSizeMaxPercentage) / 100) : 0;

        int yRandExtraSizeMaxPercentage = 100 - (roomMinSizePercentage * 100) - (roomMarginPercentage * 100) - (float)(((startPos.y - area.startPos.y) / yWidth) * 100);
        float yEndPosExtraSize = yRandExtraSizeMaxPercentage > 0 ? yWidth * (float)((rand() % yRandExtraSizeMaxPercentage) / 100) : 0;

    Vector2 endPos = {
        // base value is map section width * min size, then adds a random value between 0 and the distance from the startpos of the room to the end of the map section - margin
        startPos.x + (xWidth * roomMinSizePercentage) + xEndPosExtraSize,
        startPos.y + (yWidth * roomMinSizePercentage) + yEndPosExtraSize};
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
    *room = (rect_t){
        .startPos = startPos,
        .endPos = endPos};
}

void GenerateBSPMapSections(int iterationCount, int desiredIterations, mapSection_t *mapSection)
{
    mapSection->splitMapSections = (mapSection_t *)calloc(2, sizeof(mapSection_t));
    printf("gen iteration: %d\n", iterationCount);

    if (rand() % xySplitRandomizer >= xySplitRandomizerThreshold)
    {
        xySplitRandomizerThreshold++;
        puts("split in x");
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
        puts("split in y");
        // splits in random place with atleast a 20% margin to the left/right
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
        printf("all iterations complete: iterations: %d, desired iterations: %d\n",
               iterationCount, desiredIterations);
        GenerateRoom(&mapSection->splitMapSections[0].room, mapSection->splitMapSections[0].area);
        GenerateRoom(&mapSection->splitMapSections[1].room, mapSection->splitMapSections[1].area);
        // return;
    }
    else
    {
        iterationCount++;
        GenerateBSPMapSections(iterationCount, desiredIterations, (mapSection->splitMapSections));
        GenerateBSPMapSections(iterationCount, desiredIterations, (mapSection->splitMapSections + 1));
    }
}

int main()
{
    srand(time(NULL));
    InitWindow(screenWidth, screenHeight, "map generation test");
    SetTargetFPS(targetFPS);

    mapSection_t map = (mapSection_t){
        .area.startPos = (Vector2){0, 0},
        .area.endPos = (Vector2){screenWidth, screenHeight},
        .splitMapSections = NULL};

    GenerateBSPMapSections(0, iterations, &map);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        if (IsKeyPressed(KEY_SPACE))
        {
            GenerateBSPMapSections(0, iterations, &map);
        }
        DrawBSPMapSections(0, iterations, map, RED);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}