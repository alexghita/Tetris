// Colours used in the game, taken from http://www.flatuicolorpicker.com.
const struct colour colours[] = {
  {0.992f, 0.890f, 0.655f},
  {0.424f, 0.478f, 0.537f},
  {1.0f, 1.0f, 1.0f},
  {0.0f, 0.0f, 0.0f},
  {0.506f, 0.812f, 0.878f},
  {0.145f, 0.455f, 0.663f},
  {0.910f, 0.494f, 0.016f},
  {0.969f, 0.792f, 0.094f},
  {0.0f, 0.694f, 0.416f},
  {0.404f, 0.255f, 0.447f},
  {0.937f, 0.282f, 0.212f}
};

// Various indices for the colours array.
const int BACKGROUND = 0;
const int GREY = 1;
const int WHITE = 2;
const int BLACK = 3;
const int CYAN = 4;
const int BLUE = 5;
const int ORANGE = 6;
const int YELLOW = 7;
const int GREEN = 8;
const int PURPLE = 9;
const int RED = 10;
// Window and display sizes.
const float WINDOW_WIDTH = 860.0f;
const float WINDOW_HEIGHT = 1000.0f;
const float DISPLAY_WIDTH = 860.0f;
const float DISPLAY_HEIGHT = 1000.0f;
// Screen codes.
const int MENU = 0;
const int PREGAME = 1;
const int GAME = 2;
const int GAME_OVER = 3;
const int HIGH_SCORE = 4;
// Menu button codes.
const int PLAY_BUTTON = 0;
const int HIGH_SCORES_BUTTON = 1;
const int EXIT_BUTTON = 2;
// Maximum difficulty.
const int MAX_DIFFICULTY = 50;
/**
 * Standard block sizes, divided into inner block (the "bumped square"),
 * and the outer block (the whole square, including the shaded "sloped" edges).
 */
const float INNER_BLOCK_SIZE = 12.5f;
const float OUTER_BLOCK_SIZE = 20.0f;
// Game block sizes, i.e. the blocks that make up the game pieces.
const float GAME_BLOCK_SIZE = DISPLAY_HEIGHT / 21.0f;
const float GAME_BLOCK_SCALE = GAME_BLOCK_SIZE / OUTER_BLOCK_SIZE;
// These values are used frequently, so they are precomputed here.
const float DISPLAY_WIDTH_HALF = DISPLAY_WIDTH * 0.5f;
const float INNER_BLOCK_SIZE_HALF = INNER_BLOCK_SIZE * 0.5f;
const float OUTER_BLOCK_SIZE_HALF = OUTER_BLOCK_SIZE * 0.5f;
const float GAME_BLOCK_SIZE_HALF = GAME_BLOCK_SIZE * 0.5f;
// Line width used to draw block and window border.
const float LINE_WIDTH = 2.5f;
/**
 * Game board sizes. Visible area is 10x20, but two extra rows are added to
 * allow rotation of pieces at the top of the board.
 */
const int GAME_BOARD_WIDTH = 10;
const int GAME_BOARD_HEIGHT = 22;
// Game pieces indices.
const int I_PIECE = 0;
const int J_PIECE = 1;
const int L_PIECE = 2;
const int O_PIECE = 3;
const int S_PIECE = 4;
const int T_PIECE = 5;
const int Z_PIECE = 6;
// The coordinates for each game piece relative to its centre, {0, 0}.
const int GAME_PIECES[][2] = {
  // I piece.
  {0, 0}, {-1, 0}, {1, 0}, {2, 0},
  // J piece.
  {0, 0}, {-1, 0}, {1, 0}, {1, -1},
  // L piece.
  {0, 0}, {-1, 0}, {-1, -1}, {1, 0},
  // O piece.
  {0, 0}, {1, 0}, {0, -1}, {1,- 1},
  // S piece.
  {0, 0}, {1, 0}, {0, -1}, {-1, -1},
  // T piece.
  {0, 0}, {-1, 0}, {1, 0}, {0, -1},
  // Z piece.
  {0, 0}, {-1, 0}, {0, -1}, {1, -1}
};
