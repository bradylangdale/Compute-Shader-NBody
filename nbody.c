/*******************************************************************************************
*
*   raylib [shaders] example - Mesh instancing
*
*   Example originally created with raylib 3.7, last time updated with raylib 4.2
*
*   Example contributed by @seanpringle and reviewed by Max (@moliad) and Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2020-2023 @seanpringle, Max (@moliad) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/


#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include <stdlib.h>         // Required for: calloc(), free()

#define MAX_INSTANCES  2048

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 768;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "raylib testing");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){ -125.0f, 125.0f, -125.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };              // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };                  // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                        // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                     // Camera projection type

    // Define mesh to be instanced
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);

    // Load default material (using raylib internal default shader) for non-instanced mesh drawing
    // WARNING: Default shader enables vertex color attribute BUT GenMeshCube() does not generate vertex colors, so,
    // when drawing the color attribute is disabled and a default color value is provided as input for thevertex attribute
    Material matDefault = LoadMaterialDefault();
    matDefault.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

    // compute shader
    char *testCode = LoadFileText("resources/shaders/glsl430/test.comp");
    unsigned int testShader = rlCompileShader(testCode, RL_COMPUTE_SHADER);
    unsigned int testProgram = rlLoadComputeShaderProgram(testShader);
    UnloadFileText(testCode);

    // render shader
    const Vector2 resolution = { 768, 768 };
    Shader testRenderShader = LoadShader(NULL, "resources/shaders/glsl430/test_render.fs");
    int resUniformLoc = GetShaderLocation(testRenderShader, "resolution");

    // Load shader storage buffer object (SSBO), id returned
    unsigned int ssboA = rlLoadShaderBuffer(768*768*sizeof(Vector3), NULL, RL_DYNAMIC_COPY);
    unsigned int ssboB = rlLoadShaderBuffer(768*768*sizeof(Vector3), NULL, RL_DYNAMIC_COPY);
    unsigned int ssboC = rlLoadShaderBuffer(sizeof(unsigned int), NULL, RL_DYNAMIC_COPY);

    int dir = -2;
    unsigned int testValue[1];
    testValue[0] = 0;
    rlUpdateShaderBuffer(ssboC, &testValue, sizeof(testValue), 0);
    

    // Create a white texture of the size of the window to update
    // each pixel of the window using the fragment shader: golRenderShader
    Image whiteImage = GenImageColor(768, 768, WHITE);
    Texture whiteTex = LoadTextureFromImage(whiteImage);
    UnloadImage(whiteImage);
    //--------------------------------------------------------------------------------------


    SetTargetFPS(144);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update the light shader with the camera view position
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        
        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            // Process game of life logic
            rlEnableShader(testProgram);
            rlBindShaderBuffer(ssboA, 1);
            rlBindShaderBuffer(ssboB, 2);
            rlBindShaderBuffer(ssboC, 3);
            rlComputeShaderDispatch(768/16, 768/16, 1);
            rlDisableShader();

            testValue[0] += dir;
            //if (testValue[0] >= 768)
            //    dir *= -1;

            rlUpdateShaderBuffer(ssboC, &testValue, sizeof(testValue), 0);

            // ssboA <-> ssboB
            int temp = ssboA;
            ssboA = ssboB;
            ssboB = temp;

            rlBindShaderBuffer(ssboA, 1);
            SetShaderValue(testRenderShader, resUniformLoc, &resolution, SHADER_UNIFORM_VEC2);

            BeginShaderMode(testRenderShader);
                DrawTexture(whiteTex, 0, 0, WHITE);
            EndShaderMode();

            /*BeginMode3D(camera);

                // Draw cube mesh with default material (BLUE)
                DrawMesh(cube, matDefault, MatrixTranslate(-10.0f, 0.0f, 0.0f));

            EndMode3D();*/

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    // Unload shader buffers objects
    rlUnloadShaderBuffer(ssboA);
    rlUnloadShaderBuffer(ssboB);
    rlUnloadShaderBuffer(ssboC);

    // Unload compute shader programs
    rlUnloadShaderProgram(testProgram);

    UnloadTexture(whiteTex);            // Unload white texture
    UnloadShader(testRenderShader);      // Unload rendering fragment shader

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
