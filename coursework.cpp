#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

#include "structs.h"
#include "constants.h"
#include "util.h"

int game_board[GAME_BOARD_WIDTH][GAME_BOARD_HEIGHT];
int piece_blocks[4][2];
int current_screen = MENU;
int highlighted_button = PLAY_BUTTON;
int difficulty;
int score;
int next_piece_type;
int current_piece_type;
int slept; // How much time has passed since the last piece descent.
int pieces_spawned; // How many pieces have been spawned.
int countdown; // Time left until resuming or starting a game.
int paused_slept; // Elapsed time since last piece descent when pausing.
bool new_piece; // True if a new piece is requested.
bool grid_enabled; // True if grid view is enabled.
bool projection_enabled; // True if piece projection is enabled.
bool paused; // True if the game is paused.
bool has_high_score; // True if the current score is a high score.
vector<int> high_scores;

/**
 * Returns true if the game is running, i.e. if the current screen is the game
 * screen and the game isn't paused or resuming.
 * @return
 */
bool is_game_running() {
  return current_screen == GAME && !countdown && !paused;
}

/**
 * Returns true if the block at the given coordinates is part of the currently
 * falling piece.
 * @param x the block's column
 * @param y the block's line
 * @return true if it is part of the piece, false otherwise
 */
bool is_block_in_piece(int x, int y) {
  for (int i = 0; i < 4; i++) {
    if (piece_blocks[i][0] == x && piece_blocks[i][1] == y) {
      return true;
    }
  }
  return false;
}

/**
 * Displays a window containing the top 10 high scores sorted in descending
 * order.
 */
void display_high_scores() {
  string score;
  int counter = 1;

  // Display the window.
  display_subscreen(DISPLAY_WIDTH * 0.15f, DISPLAY_HEIGHT * 0.15f,
      DISPLAY_WIDTH * 0.85f, DISPLAY_HEIGHT * 0.85f);
  // Display the high scores.
  glPushMatrix();
    glTranslatef(0.0f, DISPLAY_HEIGHT * 0.75f, 0.0f);
    draw_text("High Scores", true, 0.6f, 0.45f);

    // Display a help message.
    glPushMatrix();
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.55f, 0.0f);
      draw_text("Press ESC to go back.", true, 0.25f, 0.2f);
    glPopMatrix();

    for (vector<int>::iterator it = high_scores.begin();
         it < high_scores.end(); it++) {
      score = int_to_string(counter) + ". " + int_to_string(*it);
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.05f, 0.0f);
      draw_text(score, true, 0.3f, 0.25f);
      counter++;
    }
  glPopMatrix();
}

/**
 * Displays a screen for when the game is over, which shows the score and
 * tells the player whether it is a high score or not.
 */
void display_game_over() {
  string score_string = "Score: " + int_to_string(score);
  float score_offset = get_offset(score_string);

  // Display the pregame window.
  display_subscreen(DISPLAY_WIDTH * 0.25f, DISPLAY_HEIGHT * 0.35f,
      DISPLAY_WIDTH * 0.75f, DISPLAY_HEIGHT * 0.75f);

  // Display the score.
  glPushMatrix();
    glTranslatef(0.0f, DISPLAY_HEIGHT * 0.65f, 0.0f);
    draw_text("Game over!", true, 0.3f, 0.25f);
    glPushMatrix();
      glTranslatef(DISPLAY_WIDTH_HALF - score_offset * 0.3f,
                   -DISPLAY_HEIGHT * 0.0625, 0.0f);
      draw_text("Score: ", false, 0.3f, 0.25f);
      glColor3f(colours[GREEN].r, colours[GREEN].g, colours[GREEN].b);
      glTranslatef(get_offset("Score: ") * 0.6f, 0.0f, 0.0f);
      draw_text(int_to_string(score), false, 0.3f, 0.25f);
    glPopMatrix();

    // Display a help message.
    glPushMatrix();
      glColor3f(colours[BLACK].r, colours[BLACK].g, colours[BLACK].b);
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.25f, 0.0f);
      draw_text("Press ENTER to continue.", true, 0.2f, 0.25f);
    glPopMatrix();

    // Display a message if there is a high score.
    if (has_high_score) {
      glColor3f(colours[RED].r, colours[RED].g, colours[RED].b);
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.15f, 0.0f);
      draw_text("New high score!", true, 0.3f, 0.25f);
    }
  glPopMatrix();
}

/**
 * Displays the countdown for starting/resuming a game.
 */
void display_game_countdown() {
  // If paused, display an appropriate message. Else, display the countdown.
  string countdown_string = paused ? "Paused" : int_to_string(countdown);
  // Scale the message appropriately.
  float scale = paused ? 1.0f : 3.0f;

  glPushMatrix();
    glTranslatef(GAME_BLOCK_SIZE * 6.0f - get_offset(countdown_string) * scale,
                 GAME_BLOCK_SIZE * 8.0f, 0.0f);
    glColor3f(colours[GREEN].r, colours[GREEN].g, colours[GREEN].b);
    draw_text(countdown_string, false, scale, scale);
  glPopMatrix();
}

/**
 * Displays the current piece's projection, i.e. where it would fall from
 * its current position.
 */
void display_game_projection() {
  int projection_distance = 25;
  int height;

  // Find the distance between the piece and the projection.
  for (int i = 0; i < 4; i++) {
    if (piece_blocks[i][1] == 0) {
      projection_distance = min(projection_distance, 1);
      continue;
    }
    height = piece_blocks[i][1];
    while (height) {
      height--;
      if (game_board[piece_blocks[i][0]][height]) {
        if (!is_block_in_piece(piece_blocks[i][0], height)) {
          projection_distance = min(projection_distance,
                                    piece_blocks[i][1] - height);
        }
        break;
      }
      if (height == 0) {
        projection_distance = min(projection_distance, piece_blocks[i][1] + 1);
      }
    }
  }

  // Print the projection blocks.
  for (int i = 0; i < 4; i++) {
    glPushMatrix();
      glTranslatef(GAME_BLOCK_SIZE * (float)(piece_blocks[i][0] + 2) -
                   GAME_BLOCK_SIZE_HALF,
                   GAME_BLOCK_SIZE * (float)(piece_blocks[i][1] -
                   projection_distance + 3) - GAME_BLOCK_SIZE_HALF, 0.0f);
      glScalef(GAME_BLOCK_SCALE, GAME_BLOCK_SCALE, 0.0f);
      glColor3f(colours[current_piece_type + CYAN].r,
                colours[current_piece_type + CYAN].g,
                colours[current_piece_type + CYAN].b);
      glBegin(GL_LINE_LOOP);
        glVertex2f(-OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF);
        glVertex2f(OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF);
        glVertex2f(OUTER_BLOCK_SIZE_HALF, -OUTER_BLOCK_SIZE_HALF);
        glVertex2f(-OUTER_BLOCK_SIZE_HALF, -OUTER_BLOCK_SIZE_HALF);
      glEnd();
    glPopMatrix();
  }
}

/**
 * Displays the game board, i.e. the squares which already contain blocks.
 */
void display_game_board() {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 20; j++) {
      if (!game_board[i][j]) {
        continue;
      }
      glPushMatrix();
        glTranslatef(GAME_BLOCK_SIZE * (float)(i + 2) - GAME_BLOCK_SIZE * 0.5f,
                     GAME_BLOCK_SIZE * (float)(j + 2) - GAME_BLOCK_SIZE * 0.5f,
                     0.0f);
        glScalef(GAME_BLOCK_SCALE, GAME_BLOCK_SCALE, 0.0f);
        draw_block(colours[game_board[i][j]]);
      glPopMatrix();
    }
  }
}

/**
 * Draws text that scales automatically to fit into the game sidebar.
 * @param text the input text
 * @param colour the colour of the text
 */
void draw_sidebar_text(string text, int colour) {
  float sidebar_width = DISPLAY_HEIGHT / 21.0f * 5.0f;
  float text_width = get_offset(text) * 2.0f;
  float text_scale_x = 0.3f;
  float text_scale_y = 0.3f;

  glPushMatrix();
    glColor3f(colours[colour].r, colours[colour].g, colours[colour].b);
    // If the text won't fit, change the scale.
    if (text_width * text_scale_x > sidebar_width) {
      text_scale_x = sidebar_width / text_width * 0.95f;
      text_scale_y = 0.25f;
    }
    glScalef(text_scale_x, text_scale_y, 0.0f);
    glTranslatef(-text_width * 0.5f, 0.0f, 0.0f);
    draw_text(text, false, 1.0f, 1.0f);
  glPopMatrix();
}

/**
 * Displays the game sidebar, which shows the next piece, the score, the
 * difficulty and the game controls.
 */
void display_game_sidebar() {
  // The number of messages to be displayed on the sidebar.
  int number_of_messages = 12;
  // The messages that will be displayed on the sidebar.
  string messages[] = {
    "Next piece:", "Score:", int_to_string(score), "Difficulty:",
    int_to_string(difficulty), "Controls:", "Arrows: move piece.",
    "Space: drop piece.", "P: pause/resume game.", "G: toggle grid view.",
    "H: toggle piece projection.", "ESC: quit."
  };
  // Downward translations that must be made between lines of text.
  float y_translations[] = {
    GAME_BLOCK_SIZE * 2.0f - DISPLAY_HEIGHT, GAME_BLOCK_SIZE * 4.0f,
    GAME_BLOCK_SIZE, GAME_BLOCK_SIZE * 1.5f, GAME_BLOCK_SIZE,
    GAME_BLOCK_SIZE * 1.5f, GAME_BLOCK_SIZE, GAME_BLOCK_SIZE,
    GAME_BLOCK_SIZE, GAME_BLOCK_SIZE, GAME_BLOCK_SIZE,
    GAME_BLOCK_SIZE * 3.5f
  };

  glPushMatrix();
    // Translate to the width centre of the sidebar.
    glTranslatef(GAME_BLOCK_SIZE * 14.5f, 0.0f, 0.0f);

    for (int i = 0; i < number_of_messages; i++) {
      glTranslatef(0.0f, -y_translations[i], 0.0f);

      // Draw the score and difficulty in green, everything else in black.
      if (i == 2 || i == 4) {
        draw_sidebar_text(messages[i], GREEN);
      } else {
        draw_sidebar_text(messages[i], BLACK);
      }

      // Display the piece lookahead.
      if (i == 0) {
        glPushMatrix();
          // Ensure the next piece is centered.
          float piece_translate_x = -GAME_BLOCK_SIZE;
          float piece_translate_y = -GAME_BLOCK_SIZE;

          if (next_piece_type == 0) {
            piece_translate_x *= 0.5f;
            piece_translate_y *= 1.5f;
          } else if (next_piece_type == 3) {
            piece_translate_x *= 0.5f;
          } else {
            piece_translate_x = 0.0f;
          }

          glTranslatef(piece_translate_x, piece_translate_y, 0.0f);
          glScalef(GAME_BLOCK_SCALE, GAME_BLOCK_SCALE, 0.0f);
          draw_piece(next_piece_type);
        glPopMatrix();
      }
    }
  glPopMatrix();
}

/**
 * Displays the border that separates the game area (where pieces are placed)
 * from the sidebar which contains information such as what the next piece is,
 * score, and help messages.
 */
void display_game_border() {
  // How many blocks make up the border.
  int height_in_blocks = 21;
  int width_in_blocks = 18;
  // Translations for drawing various parts of the border.
  float translate_x1 = OUTER_BLOCK_SIZE * 11.0f;
  float translate_x2 = OUTER_BLOCK_SIZE * 6.0f;
  float translate_y = OUTER_BLOCK_SIZE * 20.0f;

  glPushMatrix();
    glScalef(GAME_BLOCK_SCALE, GAME_BLOCK_SCALE, 0.0f);
    glTranslatef(OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF, 0.0f);

    // Draw the vertical borders.
    glPushMatrix();
      for (int i = 0; i < height_in_blocks; i++) {
        draw_block(colours[GREY]);
        glTranslatef(translate_x1, 0.0f, 0.0f);
        draw_block(colours[GREY]);
        glTranslatef(translate_x2, 0.0f, 0.0f);
        draw_block(colours[GREY]);
        glTranslatef(-translate_x1 - translate_x2, OUTER_BLOCK_SIZE, 0.0f);
      }
    glPopMatrix();

    // Draw the horizontal borders.
    for (int i = 0; i < width_in_blocks; i++) {
      draw_block(colours[GREY]);
      if (i > 11) {
        glTranslatef(0.0f, translate_y, 0.0f);
        draw_block(colours[GREY]);
        glTranslatef(0.0f, -translate_y, 0.0f);
      }
      glTranslatef(OUTER_BLOCK_SIZE, 0.0f, 0.0f);
    }

  glPopMatrix();
}

/**
 * Displays a white grid over the game area to help the player see where
 * pieces are going and how much free space there is.
 */
void display_game_grid() {
  glPushMatrix();
    glColor3f(colours[WHITE].r, colours[WHITE].g, colours[WHITE].b);
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 20; j++) {
        glPushMatrix();
          glTranslatef(GAME_BLOCK_SIZE * (float)(i + 2) -
                       GAME_BLOCK_SIZE * 0.5f,
                       GAME_BLOCK_SIZE * (float)(j + 2) -
                       GAME_BLOCK_SIZE * 0.5f, 0.0f);
          glScalef(GAME_BLOCK_SCALE, GAME_BLOCK_SCALE, 0.0f);
          // The grid is displayed by drawing frames for each game square.
          glBegin(GL_LINE_LOOP);
            glVertex2f(-OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF);
            glVertex2f(OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF);
            glVertex2f(OUTER_BLOCK_SIZE_HALF, -OUTER_BLOCK_SIZE_HALF);
            glVertex2f(-OUTER_BLOCK_SIZE_HALF, -OUTER_BLOCK_SIZE_HALF);
          glEnd();
        glPopMatrix();
      }
    }
  glPopMatrix();
}

/**
 * Displays game information, such as the game board, the current score,
 * the current difficulty, what the next piece is and instructions.
 */
void display_game() {
  glMatrixMode(GL_MODELVIEW);
  // If the game isn't paused, display the grid if enabled.
  if (grid_enabled && is_game_running()) {
    display_game_grid();
  }
  // Display the border and sidebar.
  display_game_border();
  display_game_sidebar();
  // If the game isn't paused, display the game board.
  if (is_game_running()) {
    display_game_board();
  }
  // If the game isn't paused, display the piece projection if enabled.
  if (projection_enabled && !new_piece && is_game_running()) {
    display_game_projection();
  }
  // Display the countdown for starting/resuming a game, if necessary.
  if (countdown) {
    display_game_countdown();
  }
}

/**
 * Displays the difficulty selection screen before the game starts.
 */
void display_pregame() {
  // The current difficulty as a string.
  string difficulty_string = int_to_string(difficulty);
  // How much offset is required to print the difficulty centered.
  float difficulty_offset = get_offset(difficulty_string);

  // Display the pregame screen.
  display_subscreen(DISPLAY_WIDTH * 0.25, DISPLAY_HEIGHT * 0.35f,
      DISPLAY_WIDTH * 0.75, DISPLAY_HEIGHT * 0.75);

  // Display the pregame screen content.
  glPushMatrix();
    glTranslatef(0.0f, DISPLAY_HEIGHT * 0.65f, 0.0f);

    // Print the difficulty.
    glPushMatrix();
      draw_text("Starting difficulty:", true, 0.3f, 0.25f);
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.0625f, 0.0f);
      glColor3f(colours[RED].r, colours[RED].g, colours[RED].b);
      draw_text(difficulty_string, true, 0.3f, 0.25f);
    glPopMatrix();

    // Print arrows to indicate how to adjust difficulty.
    glPushMatrix();
      glTranslatef(DISPLAY_WIDTH_HALF, -DISPLAY_HEIGHT * 0.05f, 0.0f);
      glScalef(0.6f, 0.5f, 0.0f);

      for (float sign = -1.0f; sign <= 1.0f; sign += 2.0f) {
        glPushMatrix();
          glTranslatef(OUTER_BLOCK_SIZE * 4.0f * -sign, 0.0f, 0.0f);
          glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
          draw_block(get_random_piece_colour());
          glTranslatef(OUTER_BLOCK_SIZE * sign, 0.0f, 0.0f);
          draw_block(get_random_piece_colour());
          glTranslatef(OUTER_BLOCK_SIZE * -sign, OUTER_BLOCK_SIZE * -sign,
                       0.0f);
          draw_block(get_random_piece_colour());
        glPopMatrix();
      }
    glPopMatrix();

    // Print help messages.
    glColor3f(colours[BLACK].r, colours[BLACK].g, colours[BLACK].b);
    glPushMatrix();
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.2f, 0.0f);
      draw_text("Adjust using arrow keys.", true, 0.22f, 0.2f);
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.04f, 0.0f);
      draw_text("Press ENTER to continue.", true, 0.22f, 0.2f);
      glTranslatef(0.0f, -DISPLAY_HEIGHT * 0.04f, 0.0f);
      draw_text("Press ESC to go back.", true, 0.22f, 0.2f);
    glPopMatrix();
  glPopMatrix();
}

/**
 * Prints two borders on the top and bottom sides of the screen, made out of
 * blocks of random colours. To be used when drawing the main menu.
 */
void display_menu_border() {
  // The number of blocks to be used for each border.
  int number_of_blocks = 20;
  // The required block size to fill the display width.
  float border_block_size = DISPLAY_WIDTH / (float)number_of_blocks;
  // How much to scale the blocks.
  float scale = border_block_size / OUTER_BLOCK_SIZE;
  // How much to translate between the top and bottom borders.
  float translate_y = DISPLAY_HEIGHT / scale - OUTER_BLOCK_SIZE;

  glPushMatrix();
    glScalef(scale, scale, 0.0f);
    glTranslatef(OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF, 0.0f);
    for (int i = 0; i < number_of_blocks; i++) {
      // Bottom row.
      draw_block(get_random_piece_colour());
      // Top row.
      glTranslatef(0.0f, translate_y, 0.0f);
      draw_block(get_random_piece_colour());
      // Go back up to the next position.
      glTranslatef(OUTER_BLOCK_SIZE, -translate_y, 0.0f);
    }
  glPopMatrix();
}

/**
 * Prints the game title using square blocks, like in the game pieces,
 * in random colours. To be used in the main menu.
 */
void display_menu_title() {
  // The number of blocks used in the title.
  int title_size = 51;
  // How much the blocks will be scaled.
  float scale = 1.5f;
  // The width of the title, which is 21 blocks wide.
  float title_width = 21.0f * OUTER_BLOCK_SIZE * scale;
  // The initial translations to the place where the title will be displayed.
  float translate_x = (DISPLAY_WIDTH - title_width) / (2.0f * scale) +
                      OUTER_BLOCK_SIZE * 0.5;
  float translate_y = DISPLAY_HEIGHT * 0.85f / scale;
  // The translations that need to be performed.
  float translations[][2] = {
      // T
      {translate_x, translate_y}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, 0.0f}, {-OUTER_BLOCK_SIZE, -OUTER_BLOCK_SIZE},
      {0.0f, -OUTER_BLOCK_SIZE}, {0.0f, -OUTER_BLOCK_SIZE},
      {0.0f, -OUTER_BLOCK_SIZE},
      // E
      {OUTER_BLOCK_SIZE * 3.0f, 0.0f}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, 0.0f}, {-OUTER_BLOCK_SIZE * 2.0f, OUTER_BLOCK_SIZE},
      {0.0f, OUTER_BLOCK_SIZE}, {OUTER_BLOCK_SIZE, 0.0f},
      {-OUTER_BLOCK_SIZE, OUTER_BLOCK_SIZE}, {0.0f, OUTER_BLOCK_SIZE},
      {OUTER_BLOCK_SIZE, 0.0f}, {OUTER_BLOCK_SIZE, 0.0f},
      // T
      {OUTER_BLOCK_SIZE * 2.0f, 0.0f}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, 0.0f}, {-OUTER_BLOCK_SIZE, -OUTER_BLOCK_SIZE},
      {0.0f, -OUTER_BLOCK_SIZE}, {0.0f, -OUTER_BLOCK_SIZE},
      {0.0f, -OUTER_BLOCK_SIZE},
      // R
      {OUTER_BLOCK_SIZE * 3.0f, 0.0f}, {OUTER_BLOCK_SIZE * 2.0f, 0.0f},
      {0.0f, OUTER_BLOCK_SIZE}, {-OUTER_BLOCK_SIZE * 2.0f, 0.0f},
      {0.0f, OUTER_BLOCK_SIZE}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, OUTER_BLOCK_SIZE}, {-OUTER_BLOCK_SIZE * 2.0f, 0.0f},
      {0.0f, OUTER_BLOCK_SIZE}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, 0.0f},
      // I
      {OUTER_BLOCK_SIZE * 2.0f, 0.0f}, {0.0f, -OUTER_BLOCK_SIZE},
      {0.0f, -OUTER_BLOCK_SIZE}, {0.0f, -OUTER_BLOCK_SIZE},
      {0.0f, -OUTER_BLOCK_SIZE},
      // S
      {OUTER_BLOCK_SIZE * 2.0f, 0.0f}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, 0.0f}, {0.0f, OUTER_BLOCK_SIZE},
      {0.0f, OUTER_BLOCK_SIZE}, {-OUTER_BLOCK_SIZE, 0.0f},
      {-OUTER_BLOCK_SIZE, 0.0f}, {0.0f, OUTER_BLOCK_SIZE},
      {0.0f, OUTER_BLOCK_SIZE}, {OUTER_BLOCK_SIZE, 0.0f},
      {OUTER_BLOCK_SIZE, 0.0f}
    };

  glPushMatrix();
    glScalef(scale, scale, 0.0f);
    for (int i = 0; i < title_size; i++) {
      glTranslatef(translations[i][0], translations[i][1], 0.0f);
      draw_block(get_random_piece_colour());
    }
  glPopMatrix();
}

/**
 * Displays the menu buttons, i.e. the "Play" button, which starts the game,
 * the "High Scores" button, which displays a list of high scores, and the
 * "Exit" button, which closes the program. This should only be used when
 * drawing the menu.
 */
void display_menu_buttons() {
  float button_scale_x = 25.0f;
  float button_scale_y = 4.0f;
  // Spacing between buttons.
  float button_translate_y = (OUTER_BLOCK_SIZE * (button_scale_y + 1.0)) /
                              button_scale_y;
  float text_scale_y = 0.35f;
  // The text should be centered vertically on the button.
  float text_translate_y = DISPLAY_HEIGHT * 0.2f - 18.0f;
  // Arrows should be centered vertically with the button, and on its sides.
  float arrow_translate_x = DISPLAY_WIDTH_HALF -
                            (button_scale_x * OUTER_BLOCK_SIZE / 2) * 1.1f;
  float arrow_translate_y = DISPLAY_HEIGHT * 0.2f +
                            (float)(2 - highlighted_button) *
                            button_translate_y * button_scale_y;

  // Draw the buttons as rectangular blocks.
  glPushMatrix();
    glTranslatef(DISPLAY_WIDTH_HALF, DISPLAY_HEIGHT * 0.2f, 0.0f);
    glScalef(button_scale_x, button_scale_y, 0.0f);
    draw_block(colours[RED]);
    glTranslatef(0.0f, button_translate_y, 0.0f);
    draw_block(colours[GREEN]);
    glTranslatef(0.0f, button_translate_y, 0.0f);
    draw_block(colours[BLUE]);
  glPopMatrix();

  /**
   * Write text on each button. This is done in separate matrices, as it is
   * easier to translate from origin to the required area, rather than from one
   * button to another (since the text is centered).
   * It should be noted that the x scaling of each text is hardcoded, since it
   * depends on the text length.
   */
  glPushMatrix();
    glColor3f(colours[WHITE].r, colours[WHITE].g, colours[WHITE].b);
    glTranslatef(0.0f, text_translate_y, 0.0f);
    draw_text("Exit", true, 0.8f, text_scale_y);
    glTranslatef(0.0f, button_translate_y * button_scale_y, 0.0f);
    draw_text("High Scores", true, 0.4f, text_scale_y);
    glTranslatef(0.0f, button_translate_y * button_scale_y, 0.0f);
    draw_text("Play", true, 0.8f, text_scale_y);
  glPopMatrix();

  // Display the arrows for the highlighted button.
  for (float sign = -1.0f; sign <= 1.0f; sign += 2.0f) {
    float x_translate = sign < 0.0f ? arrow_translate_x :
                                      DISPLAY_WIDTH - arrow_translate_x;
    glPushMatrix();
      glTranslatef(x_translate, arrow_translate_y, 0.0f);
      glRotatef(45.0f, 0.0f, 0.0f, -sign * 1.0f);
      draw_block(get_random_piece_colour());
      glTranslatef(sign * OUTER_BLOCK_SIZE, 0.0f, 0.0f);
      draw_block(get_random_piece_colour());
      glTranslatef(-sign * OUTER_BLOCK_SIZE, OUTER_BLOCK_SIZE, 0.0f);
      draw_block(get_random_piece_colour());
    glPopMatrix();
  }
}

/**
 * Displays the game menu, which allows the user to start a new game, look at
 * high scores, or close the game.
 */
void display_menu() {
  string help_message =
      "Press ENTER to select an option. Navigate using the arrow keys.";

  display_menu_border();
  display_menu_title();
  display_menu_buttons();

  // Print help message.
  glPushMatrix();
    glTranslatef(0.0f, OUTER_BLOCK_SIZE * 3.0f, 0.0f);
    draw_text(help_message, true, 0.185f, 0.25f);
  glPopMatrix();
}

/**
 * Spawns the piece shown in the lookahead in the sidebar.
 */
void spawn_piece() {
  // Set the coordinates for the piece blocks.
  for (int i = 0; i < 4; i++) {
    piece_blocks[i][0] = 4 + GAME_PIECES[next_piece_type * 4 + i][0];
    piece_blocks[i][1] = 19 + GAME_PIECES[next_piece_type * 4 + i][1];
  }

  // Try to spawn the piece.
  for (int i = 0; i < 4; i++) {
    // If the position is occupied, then the game is over.
    if (game_board[piece_blocks[i][0]][piece_blocks[i][1]]) {
      current_screen = GAME_OVER;
      // Determine if the score is a high score.
      if (high_scores.size() < 10) {
        high_scores.push_back(score);
        sort(high_scores.begin(), high_scores.end(), greater<int>());
        has_high_score = true;
      } else {
        for (vector<int>::iterator it = high_scores.begin();
             it < high_scores.end(); it++) {
          if (score > *it) {
            // Add the high score to the list and sort the list.
            high_scores.push_back(score);
            sort(high_scores.begin(), high_scores.end(), greater<int>());
            // Remove the lowest high score if there are more than 10.
            if (high_scores.size() > 10) {
              high_scores.pop_back();
            }
            has_high_score = true;
            break;
          }
        }
      }
      return;
    }
    game_board[piece_blocks[i][0]][piece_blocks[i][1]] = next_piece_type + CYAN;
  }

  current_piece_type = next_piece_type;
  pieces_spawned++;
  // If 10 pieces have been spawned, increase difficulty.
  if (pieces_spawned && pieces_spawned % 10 == 0 &&
      difficulty < MAX_DIFFICULTY) {
    difficulty++;
  }

  // Get the net piece type.
  next_piece_type = rand() % 7;
}

/**
 * Checks if lines have been filled, and clears them and shifts remaining
 * blocks.
 */
void clear_lines() {
  bool clear;
  // Count how many times each row must be lowered.
  int lower_row[20];
  int lines_cleared = 0;

  memset(lower_row, 0, sizeof(lower_row));

  // Check which lines need to be cleared.
  for (int j = 0; j < 20; j++) {
    clear = true;
    for (int i = 0; i < 10; i++) {
      if (!game_board[i][j]) {
        clear = false;
        break;
      }
    }
    if (clear) {
      lines_cleared++;
      for (int k = 0; k < 10; k++) {
        game_board[k][j] = 0;
      }
      // The rows above the cleared line must be shifted one line down.
      for (int k = j + 1; k < 20; k++) {
        lower_row[k]++;
      }
    }
  }

  // Shift the lines above the cleared lines.
  for (int j = 0; j < 20; j++) {
    for (int i = 0; i < 10; i++) {
      if (lower_row[j]) {
        game_board[i][j - lower_row[j]] = game_board[i][j];
        game_board[i][j] = 0;
      }
    }
  }

  // Increment the score.
  score += lines_cleared * difficulty;
}

/**
 * Performs a clockwise rotation on the current piece.
 */
void rotate_piece() {
  // If the piece is a square, do not rotate it.
  if (current_piece_type == 3) {
    return;
  }

  bool can_rotate = true;
  int centre_x = piece_blocks[0][0];
  int centre_y = piece_blocks[0][1];
  int new_piece_blocks[4][2];
  int type = game_board[piece_blocks[0][0]][piece_blocks[0][1]];
  int diff_x;
  int diff_y;

  /**
   * Compute the x and y differences between the piece blocks and the
   * piece centre, and then compute the rotated block positions.
   */
  for (int i = 0; i < 4; i++) {
    diff_x = piece_blocks[i][0] - piece_blocks[0][0];
    diff_y = piece_blocks[i][1] - piece_blocks[0][1];
    new_piece_blocks[i][0] = centre_x - diff_y;
    new_piece_blocks[i][1] = centre_y + diff_x;
  }

  // Check if the piece can be rotated.
  for (int i = 0; i < 4; i++) {
    if (new_piece_blocks[i][0] < 0 || new_piece_blocks[i][0] > 9 ||
        new_piece_blocks[i][1] < 0 || new_piece_blocks[i][1] > 21) {
      can_rotate = false;
      break;
    }
    if (game_board[new_piece_blocks[i][0]][new_piece_blocks[i][1]] &&
        !is_block_in_piece(new_piece_blocks[i][0], new_piece_blocks[i][1])) {
      can_rotate = false;
      break;
    }
  }

  if (!can_rotate) {
    return;
  }

  // Rotate the piece.
  for (int i = 0; i < 4; i++) {
    game_board[piece_blocks[i][0]][piece_blocks[i][1]] = 0;
    piece_blocks[i][0] = new_piece_blocks[i][0];
    piece_blocks[i][1] = new_piece_blocks[i][1];
  }
  for (int i = 0; i < 4; i++) {
    game_board[piece_blocks[i][0]][piece_blocks[i][1]] = type;
  }
}

/**
 * Moves the current piece in the given direction.
 * @param direction -1 = left, 0 = down, 1 = right
 */
void move_piece(int direction) {
  // If a new piece should be spawned, clear lines and spawn it.
  if (new_piece) {
    clear_lines();
    spawn_piece();
    new_piece = false;
    return;
  }

  bool move = true;
  bool same = false;
  int type = game_board[piece_blocks[0][0]][piece_blocks[0][1]];
  int move_x = direction;
  int move_y = direction == 0 ? -1 : 0;

  // Check if the piece can move in the given direction.
  for (int i = 0; i < 4; i++) {
    if (piece_blocks[i][0] + move_x < 0 || piece_blocks[i][0] + move_x > 9 ||
        piece_blocks[i][1] + move_y < 0) {
      move = false;
      break;
    }
    if (game_board[piece_blocks[i][0] + move_x][piece_blocks[i][1] + move_y] &&
        !is_block_in_piece(piece_blocks[i][0] + move_x,
        piece_blocks[i][1] + move_y)) {
      move = false;
      break;
    }
  }

  if (move) {
    // Move the piece from its original location to the new one.
    for (int i = 0; i < 4; i++) {
      game_board[piece_blocks[i][0]][piece_blocks[i][1]] = 0;
      piece_blocks[i][0] += move_x;
      piece_blocks[i][1] += move_y;
    }
    for (int i = 0; i < 4; i++) {
      game_board[piece_blocks[i][0]][piece_blocks[i][1]] = type;
    }
  } else if (direction == 0) {
    /**
     * If the piece couldn't move and it should've went down, then a new piece
     * must be spawned, since it means it reached the bottom.s
     */
    new_piece = true;
  }
}

/**
 * Moves a piece to downward until it reaches the bottom or another block.
 */
void collapse_piece() {
  while (!new_piece) {
    move_piece(0);
  }
  // Reset the timer.
  slept = 20000 + 1000 * (MAX_DIFFICULTY - difficulty);
  return;
}

/**
 * Read the pre-existing high scores from the high_scores.txt file, which
 * is located in the same directory. There should be no more than 10 high
 * scores in this file.
 */
void read_high_scores() {
  // The input file.
  ifstream input_file;
  // The current score to be read.
  int score;

  input_file.open("high_scores.txt");
  while (true) {
    input_file >> score;
    if (input_file.eof()) {
      break;
    }
    high_scores.push_back(score);
  }
  input_file.close();
}

/**
 * Write the current high score list to the file, one on each line. There
 * should be no more than 10 high scores in the list.
 */
void write_high_scores() {
  // The output file.
  ofstream output_file;

  output_file.open("high_scores.txt");
  for (vector<int>::iterator it = high_scores.begin(); it != high_scores.end();
      it++) {
    output_file << *it << "\n";
  }
  output_file.close();
}

/**
 * Sets up all the necessary variables for a new game, e.g. clearing the board
 * and resetting the score.
 */
void initialise_new_game() {
  // Reset difficulty.
  difficulty = 1;
  // Reset score.
  score = 0;
  // Reset the game board.
  memset(game_board, 0, sizeof(game_board));
  // Reset game flags.
  grid_enabled = false;
  projection_enabled = false;
  paused = false;
  has_high_score = false;
  // Reset the counter of spawned pieces.
  pieces_spawned = 0;
  // Reset the timer.
  slept = 0;
  // Reset the countdown.
  countdown = 3;
  // Request a new piece.
  new_piece = true;
  // Reset the pause timer.
  paused_slept = 20000 + 1000 * (MAX_DIFFICULTY - difficulty);
  // Get the new piece type.
  next_piece_type = rand() % 7;
}

/**
 * Display the current screen.
 */
void display() {
  // Clear the display buffer.
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  // Display the current screen.
  switch (current_screen) {
    case MENU:
      display_menu();
      break;
    case PREGAME:
      display_menu();
      display_pregame();
      break;
    case GAME:
      display_game();
      break;
    case GAME_OVER:
      display_game();
      display_game_over();
      break;
    default:
      display_menu();
      display_high_scores();
  }

  // Swap the back buffer with the front buffer.
  glutSwapBuffers();
}

/**
 * Handle regular user input, such as ENTER, ESC, and space.
 * @param key the input key
 * @param
 * @param
 */
void keyboard(unsigned char key, int, int) {
  switch (key) {
    // ENTER.
    case 13:
      // Do something based on the screen the player is in.
      switch (current_screen) {
        case MENU:
          // Select the highlighted button.
          switch (highlighted_button) {
            case PLAY_BUTTON:
              // Start a new game.
              initialise_new_game();
              current_screen = PREGAME;
              break;
            case HIGH_SCORES_BUTTON:
              // Open the high scores screen.
              current_screen = HIGH_SCORE;
              display_high_scores();
              break;
            case EXIT_BUTTON:
              // Update the high scores and close the game.
              write_high_scores();
              exit(1);
          }
          break;
        case PREGAME:
          // Start the game after selecting the difficulty.
          current_screen = GAME;
          break;
        case GAME_OVER:
          // Go back to the menu after ending the game.
          current_screen = MENU;
          break;
      }
      break;
    // ESC.
    case 27:
      // This takes the user to the menu, except during the game over screen.
      if (current_screen != GAME_OVER) {
        current_screen = MENU;
      }
      break;
    case ' ':
      // If in the game and not paused, collapse the current piece.
      if (is_game_running()) {
        collapse_piece();
      }
      break;
    case 'G':
    case 'g':
      // If in the game and not paused, toggle the grid.
      if (is_game_running()) {
        grid_enabled = !grid_enabled;
      }
      break;
    case 'H':
    case 'h':
      // If in the game and not paused, toggle the piece projection.
      if (is_game_running()) {
        projection_enabled = !projection_enabled;
      }
      break;
    case 'P':
    case 'p':
      // Pause or unpause the game.
      if (current_screen == GAME && (!countdown || paused)) {
        paused = !paused;
        if (paused) {
          paused_slept = slept;
          countdown = 3;
        }
        slept = 0;
      }
      break;
  }
  glutPostRedisplay();
}

/**
 * Handle special user input, such as the arrow keys.
 * @param key the input key
 * @param
 * @param
 */
void specialKeyboard(int key, int, int) {
  switch (key) {
    case GLUT_KEY_DOWN:
      if (current_screen == MENU) {
        // Select another button.
        highlighted_button = min(2, highlighted_button + 1);
      } else if (current_screen == GAME && !countdown && !paused) {
        move_piece(0);
      }
      break;
    case GLUT_KEY_UP:
      if (current_screen == MENU) {
        // Select another button.
        highlighted_button = max(0, highlighted_button - 1);
      } else if (current_screen == GAME && !new_piece && !countdown &&
                 !paused) {
        rotate_piece();
      }
      break;
    case GLUT_KEY_LEFT:
      if (current_screen == PREGAME) {
        // Adjust difficulty.
        difficulty = max(1, difficulty - 1);
      } else if (current_screen == GAME && !new_piece && !countdown &&
                 !paused) {
        move_piece(-1);
      }
      break;
    case GLUT_KEY_RIGHT:
      if (current_screen == PREGAME) {
        // Adjust difficulty.
        difficulty = min(10, difficulty + 1);
      } else if (current_screen == GAME && !new_piece && !countdown &&
                 !paused) {
        move_piece(1);
      }
      break;
  }

  glutPostRedisplay();
}

/**
 * Keeps track of time, in order to determine when a piece should be lowered
 * automatically.
 */
void idle() {
  // Only track time while in game.
  if (current_screen == GAME) {
    // Count elapsed time.
    usleep(1000);
    if (!paused) {
      slept += 1000;
    }
    // If in a countdown, decrement it. Otherwise, lower the piece.
    if (slept >= 20000 + 1000 * (MAX_DIFFICULTY - difficulty)) {
      if (countdown) {
        countdown--;
        slept = countdown ? 0 : paused_slept;
      } else {
        move_piece(0);
        slept = 0;
      }
    }

    glutPostRedisplay();
  }
}

/**
 * Initialise the game projection, the display size, and the clear colour.
 */
void initialise_projection() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, DISPLAY_WIDTH - 1, 0, DISPLAY_HEIGHT - 1);
	glClearColor(colours[BACKGROUND].r, colours[BACKGROUND].g,
      colours[BACKGROUND].b, 0.0f);
}

int main(int argc, char* argv[]) {
  // Read the current high scores from the high_scores.txt file.
  read_high_scores();
  // Initialise the GLUT window handler function and GL.
  glutInit(&argc, argv);
  // Use double buffering with RGBA.
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  // Main game size should be 500x1000, with 360 extra width for side bar.
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  // Create the window with an appropriate title.
  glutCreateWindow("CS324 Coursework: Tetris");
  // Set display() as the display function.
  glutDisplayFunc(display);
  // Set keyboard() as the keyboard function.
  glutKeyboardFunc(keyboard);
  // Set specialKeyboard() as the keyboard function for special characters.
  glutSpecialFunc(specialKeyboard);
  // Set idle() as the idle function.
  glutIdleFunc(idle);
  // Initialise the world projection.
  initialise_projection();
  // Go into the main loop.
  glutMainLoop();

  return 0;
}
