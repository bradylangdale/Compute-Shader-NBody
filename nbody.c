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

#include "rcamera.h"

#include "raymath.h"
#include "rlgl.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include <stdlib.h>         // Required for: calloc(), free()

#define NUM_X 50
#define NUM_Y 50
#define NUM_BODIES 4096
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1820;
    const int screenHeight = 920;

    InitWindow(screenWidth, screenHeight, "nbody testing");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){ 100.0f, 100.0f, 0.0f };    // Camera position
    camera.target = (Vector3){ 1.0f, 0.0f, 1.0f };              // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };                  // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                        // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                     // Camera projection type

    // Nbody Struct
    typedef struct Nbody
    {
        float px;
        float py;
        float pz;
        float vx;
        float vy;
        float vz;
    } Nbody;

    // compute shader
    char *nbodyCode = LoadFileText("resources/shaders/glsl430/nbody.comp");
    unsigned int nbodyShader = rlCompileShader(nbodyCode, RL_COMPUTE_SHADER);
    unsigned int nbodyProgram = rlLoadComputeShaderProgram(nbodyShader);
    UnloadFileText(nbodyCode);

    //char *collisionCode = LoadFileText("resources/shaders/glsl430/collision.comp");
    //unsigned int collisionShader = rlCompileShader(collisionCode, RL_COMPUTE_SHADER);
    //unsigned int collisionProgram = rlLoadComputeShaderProgram(collisionShader);
    //UnloadFileText(collisionCode);

    // Define transforms to be uploaded to GPU for instances
    Matrix *display_trans = (Matrix *)RL_CALLOC(NUM_BODIES, sizeof(Matrix));   // Pre-multiplied transformations passed to rlgl

    // Load shader storage buffer object (SSBO), id returned
    unsigned int nbodiesA = rlLoadShaderBuffer(NUM_BODIES*sizeof(Nbody), NULL, RL_DYNAMIC_COPY);
    unsigned int nbodiesB = rlLoadShaderBuffer(NUM_BODIES*sizeof(Nbody), NULL, RL_DYNAMIC_COPY);
    unsigned int transforms = rlLoadShaderBuffer(NUM_BODIES*sizeof(Matrix), NULL, RL_DYNAMIC_COPY);

    Nbody init_bodies[NUM_BODIES];

    for (int i = 0; i < NUM_BODIES; i++)
    {
        init_bodies[i].px = (float)GetRandomValue(-10000, 10000) / 40.0f;
        init_bodies[i].py = (float)GetRandomValue(-10000, 10000) / 100.0f;
        init_bodies[i].pz = (float)GetRandomValue(-10000, 10000) / 40.0f;
        
        float dist = sqrt(pow(init_bodies[i].px, 2) + pow(init_bodies[i].py, 2) + pow(init_bodies[i].pz, 2));
        float mag = -0.1f * dist;

        if ((float)GetRandomValue(-5, 5) > 0)
        {
            init_bodies[i].vx = mag * (-init_bodies[i].pz / dist);
            init_bodies[i].vy = 0;//(float)GetRandomValue(-50, 50);
        } else {
            init_bodies[i].vx = 0;//(float)GetRandomValue(-50, 50);
            init_bodies[i].vy = mag * (-init_bodies[i].pz / dist);
        }
        
        
        init_bodies[i].vz = mag * (init_bodies[i].px / dist);
    }

    rlUpdateShaderBuffer(nbodiesA, &init_bodies, NUM_BODIES*sizeof(Nbody), 0);

    // Define mesh to be instanced
    Mesh cube = GenMeshSphere(1.0f, 8, 16);

    //--------------------------------------------------------------------------------------
    // Load lighting shader
    Shader shader = LoadShader(TextFormat("resources/shaders/glsl%i/lighting_instancing.vs", 430),
                               TextFormat("resources/shaders/glsl%i/lighting.fs", 430));
    // Get shader locations
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");

    // Set shader value: ambient light level
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.2f, 0.2f, 0.2f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create one light
    CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 50.0f, 50.0f, 0.0f }, Vector3Zero(), WHITE, shader);

    // NOTE: We are assigning the intancing shader to material.shader
    // to be used on mesh drawing with DrawMeshInstanced()
    Material matInstances = LoadMaterialDefault();
    matInstances.shader = shader;
    matInstances.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    // Load default material (using raylib internal default shader) for non-instanced mesh drawing
    // WARNING: Default shader enables vertex color attribute BUT GenMeshCube() does not generate vertex colors, so,
    // when drawing the color attribute is disabled and a default color value is provided as input for thevertex attribute
    Material matDefault = LoadMaterialDefault();
    matDefault.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

    SetTargetFPS(33);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    
    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        // Process collisions
        //rlEnableShader(collisionProgram);
        //rlBindShaderBuffer(nbodiesA, 0);
        //rlBindShaderBuffer(nbodiesB, 1);
        //rlComputeShaderDispatch(16, 16, 16);
        //rlDisableShader();

        // ssboA <-> ssboB
        //unsigned int temp = nbodiesA;
        //nbodiesA = nbodiesB;
        //nbodiesB = temp;

        // Process nbody
        rlEnableShader(nbodyProgram);
        rlBindShaderBuffer(nbodiesA, 0);
        rlBindShaderBuffer(nbodiesB, 1);
        rlBindShaderBuffer(transforms, 2);
        rlComputeShaderDispatch(16, 16, 16);
        rlDisableShader();

        // ssboA <-> ssboB
        unsigned int temp = nbodiesA;
        nbodiesA = nbodiesB;
        nbodiesB = temp;

        rlReadShaderBuffer(transforms, display_trans, NUM_BODIES*sizeof(Matrix), 0);
        
        Vector3 pos = (Vector3){ 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < NUM_BODIES; i++)
        {
            pos.x = display_trans[i].m12;
            pos.y = display_trans[i].m13;
            pos.z = display_trans[i].m14;
        }

        pos.x /= NUM_BODIES;
        pos.y /= NUM_BODIES;
        pos.z /= NUM_BODIES;

        camera.target = pos;
        
        UpdateCamera(&camera, CAMERA_ORBITAL);

        Vector3 dir = (Vector3) {
            (camera.position.x - pos.x),
            (camera.position.y - pos.y),
            (camera.position.z - pos.z)
        };

        float dist = sqrtf(pow(dir.x, 2) + pow(dir.y, 2) + pow(dir.z, 2));

        camera.position.x += (dir.x / dist) * (500.0f - dist);
        camera.position.y += (dir.y / dist) * (500.0f - dist);
        camera.position.z += (dir.z / dist) * (500.0f - dist);

        // Update the light shader with the camera view position
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);

            BeginMode3D(camera);

                // Draw cube mesh with default material (BLUE)
                //DrawMesh(cube, matDefault, MatrixTranslate(-10.0f, 0.0f, 0.0f));
                
                // Draw meshes instanced using material containing instancing shader (RED + lighting),
                // transforms[] for the instances should be provided, they are dynamically
                // updated in GPU every frame, so we can animate the different mesh instances
                DrawMeshInstanced(cube, matInstances, display_trans, NUM_BODIES);

            EndMode3D();

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    // Unload shader buffers objects
    rlUnloadShaderBuffer(nbodiesA);
    rlUnloadShaderBuffer(nbodiesB);
    rlUnloadShaderBuffer(transforms);

    // Unload compute shader programs
    rlUnloadShaderProgram(nbodyProgram);
    //rlUnloadShaderProgram(collisionCode);
    RL_FREE(display_trans);    // Free transforms

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
