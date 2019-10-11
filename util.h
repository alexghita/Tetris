#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <string>

using namespace std;

/**
 * Returns a darker shade of the given colour.
 * @param base_colour
 * @return a 20% darker shade of the given colour
 */
struct colour get_darker_shade(struct colour base_colour) {
  struct colour darker_shade = {base_colour.r * 0.8f,
                                base_colour.g * 0.8f,
                                base_colour.b * 0.8f};
  return darker_shade;
}

/**
 * Returns a lighter shade of the given colour.
 * @param base_colour
 * @return a 20% lighter shade of the given colour
 */
struct colour get_lighter_shade(struct colour base_colour) {
  struct colour lighter_shade = {min(1.0f, base_colour.r * 1.2f),
                                 min(1.0f, base_colour.g * 1.2f),
                                 min(1.0f, base_colour.b * 1.2f)};
  return lighter_shade;
}

/**
 * Returns one of the seven colours that are used to colour game pieces (cyan,
 * blue, orange, yellow, green, purple, or red).
 * @return
 */
colour get_random_piece_colour() {
  return colours[CYAN + rand() % 7];
}

/**
 * Converts an integer to a string.
 * @param number the input number
 * @return the corresponding string
 */
string int_to_string(int number) {
  string result = "";

  do {
    result += (number % 10) + '0';
    number /= 10;
  } while (number);
  reverse(result.begin(), result.end());

  return result;
}

/**
 * Draws a block of the given colour. The block is a pseudo-3D square with
 * shaded areas and a black frame. It is used to build parts of the interface,
 * as well as the game pieces themselves.
 * @param base_colour
 */
void draw_block(struct colour base_colour) {
  // The colours used to shade the block sides.
  struct colour shades[] = {
    get_darker_shade(base_colour),
    get_darker_shade(get_darker_shade(base_colour)),
    get_lighter_shade(get_lighter_shade(base_colour)),
    get_lighter_shade(base_colour)
  };
  // The line width to be used when drawing the block border.
  float line_width = 2.5f;
  // To be used when shading the block.
  int passes = 0;

  // Draw the inner square (the non-shaded area).
  glColor3f(base_colour.r, base_colour.g, base_colour.b);
  glBegin(GL_QUADS);
    glVertex2f(-INNER_BLOCK_SIZE_HALF, INNER_BLOCK_SIZE_HALF);
    glVertex2f(INNER_BLOCK_SIZE_HALF, INNER_BLOCK_SIZE_HALF);
    glVertex2f(INNER_BLOCK_SIZE_HALF, -INNER_BLOCK_SIZE_HALF);
    glVertex2f(-INNER_BLOCK_SIZE_HALF, -INNER_BLOCK_SIZE_HALF);
  glEnd();

  /**
   * Draw the four trapeziums that make up the shaded areas. The parallel sides
   * of each trapezium are a side of the outer block and its corresponding
   * inner block side. The first two trapeziums (left and bottom) will have a
   * darker shade, while the other two (right and top) will have a lighter
   * shade (compared to the inner block).
   * This drawing has been written in a compact form using sign multiplications
   * for the coordinates of each vertex, instead of manually writing all four
   * polygon displays.
   */
  for (float sign1 = -1.0f, sign4 = -1.0f;
       sign1 <= 1.0f && sign4 <= 1.0f;
       sign1 += 2.0f, sign4 += 2.0f) {
    for (float sign2 = -1.0f, sign3 = 1.0f;
         sign2 <= 1.0f && sign3 >= -1.0f;
         sign2 += 2.0f, sign3 -= 2.0f) {
      // Pick the appropriate shade and draw the trapezium.
      glColor3f(shades[passes].r, shades[passes].g, shades[passes].b);
      glBegin(GL_POLYGON);
        glVertex2f(sign1 * OUTER_BLOCK_SIZE_HALF,
                   sign2 * OUTER_BLOCK_SIZE_HALF);
        glVertex2f(sign1 * INNER_BLOCK_SIZE_HALF,
                   sign2 * INNER_BLOCK_SIZE_HALF);
        glVertex2f(sign3 * INNER_BLOCK_SIZE_HALF,
                   sign4 * INNER_BLOCK_SIZE_HALF);
        glVertex2f(sign3 * OUTER_BLOCK_SIZE_HALF,
                   sign4 * OUTER_BLOCK_SIZE_HALF);
      glEnd();
      passes++;
    }
  }

  // Draw the black frame.
  glColor3f(colours[BLACK].r, colours[BLACK].g, colours[BLACK].b);
  glLineWidth(line_width);
  glBegin(GL_LINE_LOOP);
    glVertex2f(-OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF);
    glVertex2f(OUTER_BLOCK_SIZE_HALF, OUTER_BLOCK_SIZE_HALF);
    glVertex2f(OUTER_BLOCK_SIZE_HALF, -OUTER_BLOCK_SIZE_HALF);
    glVertex2f(-OUTER_BLOCK_SIZE_HALF, -OUTER_BLOCK_SIZE_HALF);
  glEnd();
}

/**
 * Draws a game piece of the given type.
 * @param type the piece type, ranging from 0 to 6, order as in GAME_PIECES.
 */
void draw_piece(int type) {
  // Get the corresponding colour for the given piece type.
  int colour_index = type + CYAN;

  for (int i = 0; i < 4; i++) {
    /**
     * Translate each block of the piece into its position relative to the
     * current location (i.e. the centre block of the piece), and draw it.
     */
    glPushMatrix();
      glTranslatef(GAME_PIECES[type * 4 + i][0] * OUTER_BLOCK_SIZE,
                   GAME_PIECES[type * 4 + i][1] * OUTER_BLOCK_SIZE, 0.0f);
      draw_block(colours[colour_index]);
    glPopMatrix();
  }
}

/**
 * Gets the offset required for the given text to be centred on a line.
 * @param text the input text
 * @return the offset by which the matrix must be translated before writing
 *         the given text, in order for it to be centred
 */
float get_offset(string text) {
  float text_width = 0.0f;

  for (string::iterator it = text.begin(); it != text.end(); it++) {
    text_width += glutStrokeWidth(GLUT_STROKE_ROMAN, *it);
  }
	return (text_width * 0.5f);
}

/**
 * Draws the given text using a stroke font.
 * @param text the text to be drawn.
 * @param centred whether the text should be horizontally centred.
 * @param scale_x
 * @param scale_y
 */
void draw_text(string text, bool centred, float scale_x, float scale_y) {
  glPushMatrix();
    if (centred) {
      glTranslatef(DISPLAY_WIDTH_HALF - get_offset(text) * scale_x,
                   0.0f, 0.0f);
    }
    glScalef(scale_x, scale_y, 0.0f);
    for (string::iterator it = text.begin(); it != text.end(); it++) {
      glutStrokeCharacter(GLUT_STROKE_ROMAN, *it);
    }
  glPopMatrix();
}

/**
 * Displays a subscreen, i.e. a rectangular screen over another main screen.
 * Used to draw the high scores screen, or the difficulty selection screen.
 * Subscreens are cyan, with a black border.
 * @param top_left_x
 * @param top_left_y
 * @param bottom_right_x
 * @param bottom_right_y
 */
void display_subscreen(float top_left_x, float top_left_y,
    float bottom_right_x, float bottom_right_y) {
  // Display the window.
  glColor3f(colours[CYAN].r, colours[CYAN].g, colours[CYAN].b);
  glBegin(GL_QUADS);
    glVertex2f(top_left_x, top_left_y);
    glVertex2f(bottom_right_x, top_left_y);
    glVertex2f(bottom_right_x, bottom_right_y);
    glVertex2f(top_left_x, bottom_right_y);
  glEnd();

  // Display the window border.
  glColor3f(colours[BLACK].r, colours[BLACK].g, colours[BLACK].b);
  glLineWidth(LINE_WIDTH);
  glBegin(GL_LINE_LOOP);
    glVertex2f(top_left_x, top_left_y);
    glVertex2f(bottom_right_x, top_left_y);
    glVertex2f(bottom_right_x, bottom_right_y);
    glVertex2f(top_left_x, bottom_right_y);
  glEnd();
}
