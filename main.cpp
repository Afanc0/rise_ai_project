/**
* Author: Gianfranco Romani
* Assignment: Rise of the AI
* Date due: 2023-11-18, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 1
#define LEVEL1_WIDTH 27
#define LEVEL1_HEIGHT 6

#define NUMBER_OF_ENEMIES 3
#define NUMBER_OF_ARMED 3
#define VICTORY_TEXT_LEN 7
#define LOSE_TEXT_LEN 8

#define HELP_TEXT_LEN 7


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"

// ————— GAME STATE ————— //
struct GameState
{
    Entity* player;
    Entity* enemy[NUMBER_OF_ENEMIES];
    Map* map;
    Entity* bullet[NUMBER_OF_ARMED];
    Entity* victory_text[VICTORY_TEXT_LEN];
    Entity* lose_text[LOSE_TEXT_LEN];
    Entity* help_text[HELP_TEXT_LEN];
};

// ————— CONSTANTS ————— //
const int   WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.0f,
BG_BLUE = 0.0f,
BG_GREEN = 0.0f,
BG_OPACITY = 1.0f;

const int   VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char GAME_WINDOW_NAME[] = "Rise of the AI";

const char  V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const char  SPRITESHEET_FILEPATH[] = "assets/monochrome_tilemap_transparent.png",
MAP_TILESET_FILEPATH[] = "assets/monochrome_tilemap.png",
FONT_SPRITESHEET[] = "assets/font1.png";


const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;

unsigned int LEVEL_1_DATA[] =
{
    0, 0, 0, 0, 140, 141, 142, 16, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 235, 0, 216, 0, 0, 0,
    15, 0, 270, 272, 160, 161, 162, 270, 272, 0, 0, 270, 271, 271, 272, 0, 0, 0, 0, 0, 0, 0, 235, 236, 0, 216, 0,
    35, 0, 290, 292, 185, 0, 0, 290, 297, 0, 0, 235, 236, 0, 292, 180, 0, 0, 0, 0, 0, 18, 0, 0, 295, 0, 0,
    271, 271, 290, 292, 204, 0, 0, 290, 297, 0, 0, 0, 385, 295, 292, 200, 0, 0, 275, 276, 276, 276, 276, 276, 0, 0, 216,
    0, 0, 290, 292, 205, 0, 0, 290, 0, 276, 276, 276, 276, 0, 292, 200, 0, 0, 295, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 290, 292, 0, 0, 0, 290, 0, 0, 0, 0, 0, 0, 292, 200, 0, 0, 0, 295, 0, 0, 0, 0, 0, 0, 0

};

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float   g_previous_ticks = 0.0f,
g_accumulator = 0.0f;

int g_enemies_alive = NUMBER_OF_ENEMIES;

// ————— PLAYER VARIABLES ————— //
bool player_alive = true,
     looking_right = true;

int ticks_before_end = 250;



// ————— GENERAL FUNCTIONS ————— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return texture_id;
}

void initialise()
{
    // ————— GENERAL ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 20, 20);

    // ————— PLAYER SET-UP ————— //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_entity_type(PLAYER);
    g_game_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_speed(2.5f);
    g_game_state.player->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);

    g_game_state.player->m_walking[g_game_state.player->LEFT] = new int[4] { 241, 242, 243, 244 };
    g_game_state.player->m_walking[g_game_state.player->RIGHT] = new int[4] { 241, 242, 243, 244 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
    g_game_state.player->m_animation_time = 0.0f;
    g_game_state.player->m_animation_frames = 4;
    g_game_state.player->m_animation_index = 0;
    g_game_state.player->m_animation_cols = 20;
    g_game_state.player->m_animation_rows = 20;

    // Jumping
    g_game_state.player->m_jumping_power = 6.0f;

    // ————— ENEMY SET-UP ————— //

    for (int i = 0; i < NUMBER_OF_ENEMIES; i++) {
        g_game_state.enemy[i] = new Entity();
        g_game_state.enemy[i]->set_entity_type(ENEMY);

        g_game_state.enemy[i]->set_movement(glm::vec3(0.0f));
        g_game_state.enemy[i]->set_speed(0.5f);
        g_game_state.enemy[i]->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
        g_game_state.enemy[i]->m_texture_id = load_texture(SPRITESHEET_FILEPATH);

        g_game_state.enemy[i]->m_animation_time = 0.0f;
        g_game_state.enemy[i]->m_animation_frames = 4;
        g_game_state.enemy[i]->m_animation_index = 0;
        g_game_state.enemy[i]->m_animation_cols = 20;
        g_game_state.enemy[i]->m_animation_rows = 20;
    }

    g_game_state.enemy[0]->set_ai_type(WALKER);
    g_game_state.enemy[0]->set_position(glm::vec3(8.0f, 1.0f, 0.0f));
    g_game_state.enemy[0]->m_walking[g_game_state.enemy[0]->DOWN] = new int[4] { 321, 322, 323, 324 };
    g_game_state.enemy[0]->m_animation_indices = g_game_state.enemy[0]->m_walking[g_game_state.enemy[0]->DOWN];

    g_game_state.enemy[1]->set_ai_type(PATROL);
    g_game_state.enemy[1]->set_ai_state(WALKING);
    g_game_state.enemy[1]->set_position(glm::vec3(9.25f, -2.0f, 0.0f));
    g_game_state.enemy[1]->m_walking[g_game_state.enemy[1]->DOWN] = new int[4] { 341, 342, 343, 344 };
    g_game_state.enemy[1]->m_animation_indices = g_game_state.enemy[1]->m_walking[g_game_state.enemy[1]->DOWN];

    g_game_state.enemy[2]->set_ai_type(PATROL);
    g_game_state.enemy[2]->set_ai_state(WALKING);
    g_game_state.enemy[2]->set_position(glm::vec3(19.25f, -2.0f, 0.0f));
    g_game_state.enemy[2]->m_walking[g_game_state.enemy[2]->DOWN] = new int[4] { 361, 362, 363, 364 };
    g_game_state.enemy[2]->m_animation_indices = g_game_state.enemy[2]->m_walking[g_game_state.enemy[2]->DOWN];


    // ————— BULLET PROJECTIAL ————— //
    for (int i = 0; i < NUMBER_OF_ARMED; i++) {
        g_game_state.bullet[i] = new Entity();
        g_game_state.bullet[i]->set_scale(glm::vec3(0.0f));
        g_game_state.bullet[i]->set_movement(glm::vec3(0.0f));
        g_game_state.bullet[i]->set_speed(2.5f);
        g_game_state.bullet[i]->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
        g_game_state.bullet[i]->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
      
        g_game_state.bullet[i]->m_animation_time = 0.0f;
        g_game_state.bullet[i]->m_animation_frames = 4;
        g_game_state.bullet[i]->m_animation_index = 0;
        g_game_state.bullet[i]->m_animation_cols = 20;
        g_game_state.bullet[i]->m_animation_rows = 20;
    }

    g_game_state.bullet[0]->m_walking[g_game_state.bullet[0]->DOWN] = new int[4] { 20, 20, 20, 20 };
    g_game_state.bullet[0]->m_animation_indices = g_game_state.bullet[0]->m_walking[g_game_state.bullet[0]->DOWN];
    g_game_state.bullet[0]->deactivate();

    g_game_state.bullet[1]->m_walking[g_game_state.bullet[1]->DOWN] = new int[4] { 325, 325, 325, 325 };
    g_game_state.bullet[1]->m_animation_indices = g_game_state.bullet[1]->m_walking[g_game_state.bullet[1]->DOWN];

    g_game_state.bullet[2]->m_walking[g_game_state.bullet[2]->DOWN] = new int[4] { 183, 183, 183, 183 };
    g_game_state.bullet[2]->m_animation_indices = g_game_state.bullet[2]->m_walking[g_game_state.bullet[2]->DOWN];


    // ————— VICTORY TEXT ————— //
    int win_text_array[] = { 89, 79, 85, 9, 87, 73, 78 };
    for (int i = 0; i < VICTORY_TEXT_LEN; i++) {
        g_game_state.victory_text[i] = new Entity();
        g_game_state.victory_text[i]->set_scale(glm::vec3(0.0f));
        g_game_state.victory_text[i]->m_texture_id = load_texture(FONT_SPRITESHEET);

        g_game_state.victory_text[i]->m_animation_time = 0.0f;
        g_game_state.victory_text[i]->m_animation_frames = 4;
        g_game_state.victory_text[i]->m_animation_index = 0;
        g_game_state.victory_text[i]->m_animation_cols = 16;
        g_game_state.victory_text[i]->m_animation_rows = 16;

        g_game_state.victory_text[i]->m_animation_indices = new int[1];
        g_game_state.victory_text[i]->m_animation_indices[0] = win_text_array[i];
    }


    // ————— LOSE TEXT ————— //
    int lose_text_array[] = { 89, 79, 85, 9, 76, 79, 83, 69 };
    for (int i = 0; i < LOSE_TEXT_LEN; i++) {
        g_game_state.lose_text[i] = new Entity();
        g_game_state.lose_text[i]->set_scale(glm::vec3(0.0f));
        g_game_state.lose_text[i]->m_texture_id = load_texture(FONT_SPRITESHEET);

        g_game_state.lose_text[i]->m_animation_time = 0.0f;
        g_game_state.lose_text[i]->m_animation_frames = 4;
        g_game_state.lose_text[i]->m_animation_index = 0;
        g_game_state.lose_text[i]->m_animation_cols = 16;
        g_game_state.lose_text[i]->m_animation_rows = 16;

        g_game_state.lose_text[i]->m_animation_indices = new int[1];
        g_game_state.lose_text[i]->m_animation_indices[0] = lose_text_array[i];
    }

    // ————— HELP TEXT ————— //
    float text_position = -3.0f;
    int help_text_array[] = { 80, 114, 101, 115, 115, 9, 71 };
    for (int i = 0; i < HELP_TEXT_LEN; i++) {
        g_game_state.help_text[i] = new Entity();
        g_game_state.help_text[i]->set_position(glm::vec3(text_position++, 3.0f, 0.0f));
        g_game_state.help_text[i]->m_texture_id = load_texture(FONT_SPRITESHEET);

        g_game_state.help_text[i]->m_animation_time = 0.0f;
        g_game_state.help_text[i]->m_animation_frames = 4;
        g_game_state.help_text[i]->m_animation_index = 0;
        g_game_state.help_text[i]->m_animation_cols = 16;
        g_game_state.help_text[i]->m_animation_rows = 16;

        g_game_state.help_text[i]->m_animation_indices = new int[1];
        g_game_state.help_text[i]->m_animation_indices[0] = help_text_array[i];
    }


    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->m_collided_bottom && player_alive)
                {
                    g_game_state.player->m_is_jumping = true;
                }
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT] && player_alive)
    {
        g_game_state.player->move_left();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
        g_game_state.player->set_texture_flip(true);
        looking_right = false;
    }
    else if (key_state[SDL_SCANCODE_RIGHT] && player_alive)
    {
       g_game_state.player->move_right();
       g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
       g_game_state.player->set_texture_flip(false);
       looking_right = true;
    }

    if (key_state[SDL_SCANCODE_G] && player_alive)
    {
        g_game_state.bullet[0]->activate();
        if (looking_right) {
            g_game_state.bullet[0]->set_position(glm::vec3(g_game_state.player->get_position().x + 1.0f, g_game_state.player->get_position().y, 0.0f));
            g_game_state.bullet[0]->set_scale(glm::vec3(1.0f));
        }
        else {
            g_game_state.bullet[0]->set_position(glm::vec3(g_game_state.player->get_position().x - 1.0f, g_game_state.player->get_position().y, 0.0f));
            g_game_state.bullet[0]->set_scale(glm::vec3(1.0f));
        }
    }

    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    g_game_state.enemy[0]->ai_activate(g_game_state.player, 0);
    g_game_state.enemy[1]->ai_activate(g_game_state.player, 80, g_game_state.bullet[1], true);
    g_game_state.enemy[2]->ai_activate(g_game_state.player, 275, g_game_state.bullet[2], false, 1.6f);

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, NULL, 0, g_game_state.map);

        for (int i = 0; i < VICTORY_TEXT_LEN; i++) {
            g_game_state.victory_text[i]->update(FIXED_TIMESTEP, g_game_state.victory_text[i], NULL, 0, g_game_state.map);
        }

        for (int i = 0; i < HELP_TEXT_LEN; i++) {
            g_game_state.help_text[i]->update(FIXED_TIMESTEP, g_game_state.help_text[i], NULL, 0, g_game_state.map);
        }

        for (int i = 0; i < LOSE_TEXT_LEN; i++) {
            g_game_state.lose_text[i]->update(FIXED_TIMESTEP, g_game_state.lose_text[i], NULL, 0, g_game_state.map);
        }

        for (int i = 0; i < NUMBER_OF_ARMED; i++) {
            g_game_state.bullet[i]->update(FIXED_TIMESTEP, g_game_state.bullet[i], NULL, 0, g_game_state.map);
        }

        for (int i = 0; i < NUMBER_OF_ENEMIES; i++) {
            g_game_state.enemy[i]->update(FIXED_TIMESTEP, g_game_state.enemy[i], NULL, 0, g_game_state.map);

            // Enemy death condition
            if (g_game_state.enemy[i]->check_collision(g_game_state.bullet[0]) || 
                (g_game_state.enemy[i]->check_collision(g_game_state.player) && !g_game_state.player->m_collided_bottom) && player_alive)
            {
                g_game_state.enemy[i]->deactivate();
                if (i != 0) {
                    g_game_state.bullet[i]->deactivate();
                }
                --g_enemies_alive;
            }

            if (g_game_state.enemy[i]->get_position().y < -4.5f && g_game_state.enemy[i]->get_is_active()) {
                g_game_state.enemy[i]->deactivate();
                if (i != 0) {
                    g_game_state.bullet[i]->deactivate();
                }
                --g_enemies_alive;
            }

            if (g_enemies_alive == 0) {
                float text_position = g_game_state.player->get_position().x - 3.0f;
                for (int i = 0; i < VICTORY_TEXT_LEN; i++) {
                    g_game_state.victory_text[i]->set_scale(glm::vec3(1.0f));
                    g_game_state.victory_text[i]->set_position(glm::vec3(text_position++, 2.0f, 0.0f));
                }
                g_game_state.player->deactivate();
                g_game_state.bullet[0]->deactivate();
                --ticks_before_end;
                if (ticks_before_end == 0) {
                    g_game_is_running = false;
                }
            }

            // Player death condition
            if (g_game_state.player->check_collision(g_game_state.enemy[i]) || 
                g_game_state.player->check_collision(g_game_state.bullet[1]) || g_game_state.player->check_collision(g_game_state.bullet[2])) 
            {
                player_alive = false;
                g_game_state.player->m_walking[g_game_state.player->RIGHT] = new int[4] { 246, 246, 246, 246 };
                g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
            }

            if (g_game_state.player->get_position().y < -4.5) 
            {
                player_alive = false;
            }

            if (!player_alive) {
                float text_position = g_game_state.player->get_position().x - 3.5f;
                for (int i = 0; i < LOSE_TEXT_LEN; i++) {
                    g_game_state.lose_text[i]->set_scale(glm::vec3(1.0f));
                    g_game_state.lose_text[i]->set_position(glm::vec3(text_position++, 2.0f, 0.0f));
                }
                g_game_state.player->deactivate();
                g_game_state.bullet[0]->deactivate();
                --ticks_before_end;
                if (ticks_before_end == 0) {
                    g_game_is_running = false;
                }
            }
        }
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 0.0f, 0.0f));

}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);

    glClear(GL_COLOR_BUFFER_BIT);

    g_game_state.map->render(&g_shader_program);

    g_game_state.player->render(&g_shader_program);

    for (int i = 0; i < VICTORY_TEXT_LEN; i++) {
        g_game_state.victory_text[i]->render(&g_shader_program);
    }

    for (int i = 0; i < LOSE_TEXT_LEN; i++) {
        g_game_state.lose_text[i]->render(&g_shader_program);
    }

    for (int i = 0; i < HELP_TEXT_LEN; i++) {
        g_game_state.help_text[i]->render(&g_shader_program);
    }

    for (int i = 0; i < NUMBER_OF_ARMED; i++) {
        g_game_state.bullet[i]->render(&g_shader_program);
    }

    for (int i = 0; i < NUMBER_OF_ENEMIES; i++) {
        g_game_state.enemy[i]->render(&g_shader_program);
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    // delete[] g_game_state.enemies;
    delete    g_game_state.player;
    delete    g_game_state.map;
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}