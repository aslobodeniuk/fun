/*
 * Copyright (C) 2025 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 */

// Compile with: gcc jpegdec_shader.c -lglfw -lGL -lGLEW -lm -o jpegdec_shader
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

const char* vertexPassThrough = "#version 300 es\n"
"layout (location = 0) in vec2 pos;\n"
"layout (location = 1) in vec2 tex;\n"
"out vec2 texCoord;\n"
"void main() {\n"
"    gl_Position = vec4(pos, 0.0, 1.0);\n"
"    texCoord = tex;\n"
"}";

static const char * fragmentPassThrough =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "in vec2 texCoord;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D rgbTex;\n"
    "void main() {\n"
    "  fragColor = texture(rgbTex, texCoord);\n"
    "}\n";

static const char * zigzagToDCT =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "in vec2 texCoord;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D zigzagInpP;\n"
    "uniform float qTable[64];\n"
    "const int zigzag8x8[64] = int[64](\n"
    "0,  1,  8, 16,  9,  2,  3, 10,\n"
    "17, 24, 32, 25, 18, 11,  4,  5,\n"
    "12, 19, 26, 33, 40, 48, 41, 34,\n"
    "27, 20, 13,  6,  7, 14, 21, 28,\n"
    "35, 42, 49, 56, 57, 50, 43, 36,\n"
    "29, 22, 15, 23, 30, 37, 44, 51,\n"
    "58, 59, 52, 45, 38, 31, 39, 46,\n"
    "53, 60, 61, 54, 47, 55, 62, 63\n"
    ");\n"
    
    "void main() {\n"
    "  ivec2 outPixel = ivec2(gl_FragCoord.xy);"
    // oook, so we need to fetch now a pixel of the same block,
    // but a different one inside of the block.
    // We only play on offset over texCoord.x
    // texCoord.y will be the same.
    // zzj represents position inside the block
    "  int zzj = outPixel.x % 64;\n"
    "  ivec2 pos = ivec2("
    "    outPixel.x - zzj + zigzag8x8[zzj],"
    "    outPixel.y"
    "  );\n"
    /* FIXME: how to get rid of that 100.0 */
    "  float dequant = qTable[zzj] / 100.0;\n"
    "  float pixel = texelFetch(zigzagInpP, pos, 0).r;\n"
    "  pixel *= dequant;\n"
    "  fragColor = vec4(pixel, 0.0, 0.0, 1.0);\n"    
    "}\n"; // validated

const char* fragmentIDCTtoRGB = "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D dctInpY;\n"
    "uniform sampler2D dctInpU;\n"
    "uniform sampler2D dctInpV;\n"
    "const float M_PI = 3.14159265358979323846;\n"
    // IDCT funcs
    "float idct_sum (float coeff, int x, int y, int xk, int yk)\n"
    "{\n"
    "  float ck = (xk == 0) ? (1.0f / sqrt(2.0f)) : 1.0f;\n"
    "  float cl = (yk == 0) ? (1.0f / sqrt(2.0f)) : 1.0f;\n"
    "  return ck * cl * coeff *  \n"
    "      cos((M_PI * (2.0f * float(x) + 1.0f) * float(xk)) / (2.0f * 8.0f)) *  \n"
    "      cos((M_PI * (2.0f * float (y) + 1.0f) * float(yk)) / (2.0f * 8.0f));\n"
    "}\n"

    // so width is now divisible by 64 :(
    // OOOK, so the
    // BX - is an X of the OUTPUT block
    // BY - is an Y of the OUTPUT block
    // X - is an X (0..8) WITHIN tha block
    // Y - is an Y (0..8) WITHIN tha bloook

    // bpl means blocks per line.
    // we say ok, the total index of the pixel / bpl is the fetch y
#define DEF_IDCT_FOR(yuv)                                               \
    "float apply_idct_for_" yuv "(int input_block_x, int input_y, int x, int y)\n" \
        "{\n"                                                           \
        "  float result = 0.0f\n;"                                      \
        "  for (int yk = 0; yk < 8; yk++) {\n"                             \
        "    for (int xk = 0; xk < 8; xk++) {\n"                           \
        "      int xxx = input_block_x + yk*8 + xk;\n"                    \
        "      float coeff = texelFetch(dctInp" yuv ", ivec2(xxx, input_y), 0).r;\n" \
        "      result += idct_sum (coeff, x, y, xk, yk);\n"               \
        "    }\n"                                                       \
        "  }\n"                                                         \
        "  return result * 0.25f;\n"                                    \
        "}\n"
    
    DEF_IDCT_FOR ("Y")
    DEF_IDCT_FOR ("U")
    DEF_IDCT_FOR ("V")
    
    // YUV to RGB
    "    vec4 yuv_to_rgb (float y, float u, float v)\n"
    "    {\n"
    "      float r = y + 1.402 * v;\n"
    "      float g = y - 0.344136 * u - 0.714136 * v;\n"
    "      float b = y + 1.772 * u;\n"
    "      return vec4(r, g, b, 1.0);\n"  
    "    }\n"
    
    "void main() {\n"
    // output position
    "    ivec2 outPixel = ivec2(gl_FragCoord.xy);"
    "    int width = textureSize (dctInpY, 0).x;\n"
    // block per line __of the input texture__
    "    int ibpl = width / 64;\n"    
    // global block index.
    // ok, so to remap from the output to input we need
    // to multiply this global index by proper numbers
    // Let's think.
    "    int obpl = width / 8;\n"
    // This is the index!!
    "    int gbi = (outPixel.x / 8) + (outPixel.y / 8) * obpl;"
    // so gbi / bpl is the input block y, that is constant
    "    int input_y = gbi / ibpl;\n"
    "    int input_block_x = (gbi - (input_y * ibpl)) * 64;"
    
    // Position within the 8x8 block
    "    int x = outPixel.x % 8;\n"
    "    int y = outPixel.y % 8;\n"
    // do idct
    "    float py = apply_idct_for_Y(input_block_x, input_y, x, y);\n"
    "    float pu = apply_idct_for_U(input_block_x, input_y, x, y) - 0.5;\n"
    "    float pv = apply_idct_for_V(input_block_x, input_y, x, y) - 0.5;\n"
    "    fragColor = yuv_to_rgb (py, pu, pv);\n"
    "}\n";

typedef struct
{
  void *Y;
  void *U;
  void *V;
  int width;
  int height;
} RFYUVData;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BLOCK_SIZE 8

const float losslessQuant[64] = {
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0
};

const int zigzag8x8[64] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

void unzigzag_block(const float in[64], float out[64]) {
    for (int i = 0; i < 64; i++) {
        out[zigzag8x8[i]] = in[i];
    }
}

void zigzag_block(const float in[64], float out[64]) {
    for (int i = 0; i < 64; i++) {
        out[i] = in[zigzag8x8[i]];
    }
}

void unzigzag_image(const float *input, float *output, int width, int height) {
  int numBlocks = width * height / (8*8);
  for (int block = 0; block < numBlocks; block++) {
    unzigzag_block(&input[block * 64], &output[block * 64]);
  }
}

void zigzag_image(const float *input, float *output, int width, int height) {
  int numBlocks = width * height / (8*8);
  for (int block = 0; block < numBlocks; block++) {
    zigzag_block(&input[block * 64], &output[block * 64]);
  }
}

void quant_block(const float table[64], const float in[64], float out[64]) {
    for (int i = 0; i < 64; i++) {
      out[i] = roundf(in[i] * 100.0 / table[i]);
    }
}

void quant_image(const float table[64],
    const float *input, float *output, int width, int height) {
  int numBlocks = width * height / (8*8);
  for (int block = 0; block < numBlocks; block++) {
    quant_block(table, &input[block * 64], &output[block * 64]);
  }
}

void dct8x8_block(const float *input, float *output, int stride) {
    for (int u = 0; u < BLOCK_SIZE; u++) {
        for (int v = 0; v < BLOCK_SIZE; v++) {
            float sum = 0.0f;
            for (int x = 0; x < BLOCK_SIZE; x++) {
                for (int y = 0; y < BLOCK_SIZE; y++) {
                    float pixel = input[y * stride + x];
                    sum += pixel *
                        cosf(((2 * x + 1) * u * M_PI) / 16.0f) *
                        cosf(((2 * y + 1) * v * M_PI) / 16.0f);
                }
            }

            // Scaling factors for the DC component
            float cu = (u == 0) ? 1.0f / sqrtf(2.0f) : 1.0f;
            float cv = (v == 0) ? 1.0f / sqrtf(2.0f) : 1.0f;

            // Apply normalization factor
            output[v * 8 + u] = 0.25f * cu * cv * sum;
        }
    }
}

void dct_image(const float *input, float *output, int width, int height) {
  int p = 0;
  for (int by = 0; by < height; by += 8) {
    for (int bx = 0; bx < width; bx += 8) {
      const float *block_in = &input[by * width + bx];
      float *block_out = &output[p];
      /* so we read the input respecting the strides, but write an output as
       * a sequence of [8x8] chunks. */
      dct8x8_block(block_in, block_out, width);

      p += 64;
    }
  }
}


#if 0
void idct8x8_block(const float *input, float *output, int stride) {
    for (int y = 0; y < BLOCK_SIZE; y++) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            output[y * stride + x] = apply_idct_for_pixel((float (*)[8])input, x, y);
        }
    }
}

void idct_image(float *input, float *output, int width, int height) {
  // Actuall input is the plane [8x8][8x8][8x8] sequence (and that's ok)

  int p = 0;
  for (int by = 0; by < height; by += 8) {
    for (int bx = 0; bx < width; bx += 8) {
      const float *block_in = &input[p];
      float *block_out = &output[by * width + bx];
    
      idct8x8_block (block_in, block_out, width);
      p += 64;
    }
  }
}
#endif

RFYUVData * rf_dct_that_thing (RFYUVData * float_pixels)
{
  static RFYUVData yep;
  int psize = float_pixels->width * float_pixels->height * sizeof (GL_FLOAT);

  yep.Y = malloc (psize);
  yep.U = malloc (psize);
  yep.V = malloc (psize);
  yep.width = float_pixels->width;
  yep.height = float_pixels->height;

  dct_image ((float*)float_pixels->Y, (float*)yep.Y, yep.width, yep.height);
  dct_image ((float*)float_pixels->U, (float*)yep.U, yep.width, yep.height);
  dct_image ((float*)float_pixels->V, (float*)yep.V, yep.width, yep.height);
  
  return &yep;
}

RFYUVData * rf_quant_that_thing (const float table[64], RFYUVData * float_pixels)
{
  static RFYUVData yep;
  int psize = float_pixels->width * float_pixels->height * sizeof (GL_FLOAT);

  yep.Y = malloc (psize);
  yep.U = malloc (psize);
  yep.V = malloc (psize);
  yep.width = float_pixels->width;
  yep.height = float_pixels->height;

  quant_image (table, (float*)float_pixels->Y, (float*)yep.Y, yep.width, yep.height);
  quant_image (table, (float*)float_pixels->U, (float*)yep.U, yep.width, yep.height);
  quant_image (table, (float*)float_pixels->V, (float*)yep.V, yep.width, yep.height);
  
  return &yep;
}

RFYUVData * rf_zigzag_that_thing (RFYUVData * float_pixels)
{
  static RFYUVData yep;
  int psize = float_pixels->width * float_pixels->height * sizeof (GL_FLOAT);

  yep.Y = malloc (psize);
  yep.U = malloc (psize);
  yep.V = malloc (psize);
  yep.width = float_pixels->width;
  yep.height = float_pixels->height;

  zigzag_image ((float*)float_pixels->Y, (float*)yep.Y, yep.width, yep.height);
  zigzag_image ((float*)float_pixels->U, (float*)yep.U, yep.width, yep.height);
  zigzag_image ((float*)float_pixels->V, (float*)yep.V, yep.width, yep.height);
  
  return &yep;
}

RFYUVData * rf_unzigzag_that_thing (RFYUVData * float_pixels)
{
  static RFYUVData yep;
  int psize = float_pixels->width * float_pixels->height * sizeof (GL_FLOAT);

  yep.Y = malloc (psize);
  yep.U = malloc (psize);
  yep.V = malloc (psize);
  yep.width = float_pixels->width;
  yep.height = float_pixels->height;

  unzigzag_image ((float*)float_pixels->Y, (float*)yep.Y, yep.width, yep.height);
  unzigzag_image ((float*)float_pixels->U, (float*)yep.U, yep.width, yep.height);
  unzigzag_image ((float*)float_pixels->V, (float*)yep.V, yep.width, yep.height);
  
  return &yep;
}

#if 0
RFYUVData * rf_idct_that_thing (RFYUVData * float_dcts)
{
  static RFYUVData yep;
  int psize = float_dcts->width * float_dcts->height * sizeof (GL_FLOAT);

  yep.Y = malloc (psize);
  yep.U = malloc (psize);
  yep.V = malloc (psize);
  yep.width = float_dcts->width;
  yep.height = float_dcts->height;

  idct_image ((float*)float_dcts->Y, (float*)yep.Y, yep.width, yep.height);
  idct_image ((float*)float_dcts->U, (float*)yep.U, yep.width, yep.height);
  idct_image ((float*)float_dcts->V, (float*)yep.V, yep.width, yep.height);
  
  return &yep;  
}
#endif

RFYUVData * generateYUVGradient() {

#define WIDTH 512
#define HEIGHT 512
  
  static float Y_DATA[WIDTH * HEIGHT];
  static float U_DATA[WIDTH * HEIGHT];
  static float V_DATA[WIDTH * HEIGHT];

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int i = y * WIDTH + x;
            Y_DATA[i] = (float)x*y / (WIDTH*HEIGHT);
            U_DATA[i] = 200.0/255.0;
            V_DATA[i] = 200.0/255.0;
        }          
    }

    /* our funny malloc */
    static RFYUVData ret;

    ret.Y = Y_DATA;
    ret.U = U_DATA;
    ret.V = V_DATA;
    ret.width = WIDTH;
    ret.height = HEIGHT;
    return &ret;
}

GLuint rf_create_texture(const float* data, int width, int height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // upload
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
    GLint maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    printf("Max texture size: %d\n", maxTexSize);
    
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
      printf("OpenGL error: %x\n", err);
      exit (1);
    }

    // unbind
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void rf_shader_error (GLuint shader)
{
    // Get the length of the info log
#if 1
    GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    // Allocate space for the log and retrieve it
    char *log = (char *)malloc(logLength);
    glGetShaderInfoLog(shader, logLength, NULL, log);

    // Print the log
    printf("Shader compile error:\n%s\n", log);

    free(log);
#else
  
  GLint logLength;
  glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
  char *log = (char *)malloc(logLength);
  glGetProgramInfoLog(shader, logLength, NULL, log);
  printf("Program failed:\n%s\n", log);
  free(log);
#endif

  exit (1);
}

GLuint rf_compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
      rf_shader_error (shader);
    }
    
    return shader;
}

GLuint rf_gen_target_buffer ()
{
  static const float quad[] = {
    // pos      // tex
    -1, -1,     0, 0,
    1, -1,     1, 0,
    1,  1,     1, 1,
    -1,  1,     0, 1
  };
  static const unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

  GLuint vao, vbo, ebo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  return vao;
}

typedef struct {
  const char *name;
  uint64_t thing;
  int amount;
} RFUniform;

GLuint rf_create_shader_program (const char *vertex, const char *fragment, RFUniform *unis)
{
  GLuint vs = rf_compile_shader(GL_VERTEX_SHADER, vertex);
  GLuint fs = rf_compile_shader(GL_FRAGMENT_SHADER, fragment);
  GLuint shader = glCreateProgram();
  glAttachShader(shader, vs);
  glAttachShader(shader, fs);
  glLinkProgram(shader);

  GLint linkStatus;
  glGetProgramiv(shader, GL_LINK_STATUS, &linkStatus);
  if (!linkStatus) {
    rf_shader_error (shader);
  }

  glUseProgram(shader);
  int t = 0;
  for (int i = 0; unis[i].name != NULL; i++) {
    GLint location = glGetUniformLocation(shader, unis[i].name);
    if (unis[i].amount == 1) {
      /* If amount is 1 we assume it's texture if not - array of floats */
      glUniform1i(location, t++);
    } else {
      glUniform1fv(location, unis[i].amount, (float*)unis[i].thing);
    }
  }

  return shader;
}

GLFWwindow* rf_create_window ()
{
  if (!glfwInit()) exit (1);

  GLFWwindow* window = glfwCreateWindow(1024, 1024, "JPEG Decoder Shader", NULL, NULL);
  if (!window) exit (1);
  glfwMakeContextCurrent(window);
  glewInit();

  printf ("GL: %s\n", glGetString(GL_VERSION));
  return window;
}

void rf_use_shader_program (GLenum tt, GLuint shader, RFUniform *unis)
{
  glUseProgram(shader);
  int t = 0;
  for (int i = 0; unis[i].name != NULL; i++) {
    /* if amount != 1 it's not a texture but an array */
    if (unis[i].amount != 1)
      continue;
    
    glActiveTexture(GL_TEXTURE0 + t);
    glBindTexture(tt, unis[i].thing);
    t++;
  }  
}

void rf_draw_to_target_buffer (GLuint vao)
{
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

typedef struct {
  GLuint framebuffer, texture;
} RFFb;

RFFb rf_make_framebuffer (GLenum format, int width, int height)
{
  RFFb ret;

  GLenum err;  
  
  glGenFramebuffers(1, &ret.framebuffer);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf ("incomplete\n");
    exit (1);
  }

  glGenTextures(1, &ret.texture);
  glBindFramebuffer(GL_FRAMEBUFFER, ret.framebuffer);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ret.texture);
  
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
      format == GL_RGB ? GL_RGB : GL_RED,
      format == GL_RGB ? GL_UNSIGNED_BYTE : GL_HALF_FLOAT, NULL);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ret.texture, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  return ret;
}

float half_to_float(uint16_t h) {
    uint16_t h_exp = (h & 0x7C00) >> 10;  // exponent
    uint16_t h_sig = h & 0x03FF;         // mantissa
    uint32_t sign = (h & 0x8000) << 16;  // sign << 16 to match float format

    uint32_t f;

    if (h_exp == 0) {
        // Subnormal or zero
        if (h_sig == 0) {
            f = sign;
        } else {
            // Normalize subnormal
            h_exp = 1;
            while ((h_sig & 0x0400) == 0) {
                h_sig <<= 1;
                h_exp--;
            }
            h_sig &= 0x03FF;
            h_exp += 127 - 15;
            f = sign | (h_exp << 23) | (h_sig << 13);
        }
    } else if (h_exp == 0x1F) {
        // Inf or NaN
        f = sign | 0x7F800000 | (h_sig << 13);
    } else {
        // Normalized
        uint32_t exp = h_exp + (127 - 15);
        f = sign | (exp << 23) | (h_sig << 13);
    }

    float result;
    *(uint32_t*)&result = f;
    return result;
}

void print_line_r_tex (int line, int width, int height) {
        static int once = 0;
      if (once == 0)
      {
        once = 1;
        uint16_t *printarray = malloc (sizeof (uint16_t) * width * height);
        glReadPixels(0, 0, width, height, GL_RED, GL_HALF_FLOAT,
          printarray);

        printf ("Unzigzagged Y:\n");
        for (int y = line; y == line; y++) {
          for (int x = 0; x < width; x++) {
            printf ("[%d][%d] = %f\n", x, y, half_to_float (printarray[width * y + x]));
          }
          printf ("------------------\n");
        }

        free (printarray);
      }

}

int main() {
  RFYUVData* cpu_data =
      rf_zigzag_that_thing (
        rf_quant_that_thing (losslessQuant,
                            rf_dct_that_thing (
                                               generateYUVGradient()
                              )))
      ;


  GLFWwindow* window = rf_create_window ();
  glViewport(0, 0, 1024, 1024);
  
  /* Upload CPU data to textures */
  GLuint zigzagInpY = rf_create_texture(cpu_data->Y, cpu_data->width, cpu_data->height);
  GLuint zigzagInpU = rf_create_texture(cpu_data->U, cpu_data->width, cpu_data->height);
  GLuint zigzagInpV = rf_create_texture(cpu_data->V, cpu_data->width, cpu_data->height);

  RFUniform zigzag_to_dct_unis_y[] = {
    { "zigzagInpP", zigzagInpY, 1 },
    { "qTable", (uint64_t)losslessQuant, 64 },
    { NULL }
  };

  RFUniform zigzag_to_dct_unis_u[] = {
    { "zigzagInpP", zigzagInpU, 1 },
    { "qTable", (uint64_t)losslessQuant, 64 },
    { NULL }
  };

  RFUniform zigzag_to_dct_unis_v[] = {
    { "zigzagInpP", zigzagInpV, 1 },
    { "qTable", (uint64_t)losslessQuant, 64 },
    { NULL }
  };

  /* We create 3 shaders of the same source, but the benefit is that we don't
   * have to reset the uniform.
   * The redundancy is that we could create 3 programs of 1 shader.
   * Also always reuse the vertex one. */
  GLuint dequant_shader_y = rf_create_shader_program (vertexPassThrough,
      zigzagToDCT, zigzag_to_dct_unis_y);
  GLuint dequant_shader_u = rf_create_shader_program (vertexPassThrough,
      zigzagToDCT, zigzag_to_dct_unis_u);
  GLuint dequant_shader_v = rf_create_shader_program (vertexPassThrough,
      zigzagToDCT, zigzag_to_dct_unis_v);
  
  RFFb dequant_output_y = rf_make_framebuffer (GL_R16F, cpu_data->width, cpu_data->height);
  RFFb dequant_output_u = rf_make_framebuffer (GL_R16F, cpu_data->width, cpu_data->height);
  RFFb dequant_output_v = rf_make_framebuffer (GL_R16F, cpu_data->width, cpu_data->height);
  
    // so we say, ok, input textures are attached to the shader.
    // the output one depends on the framebuffer bound.
    // then we probably need 3 framebuffers to generate some input for it in GPU
    RFUniform idct_to_rgb_unis[] = {
      { "dctInpY", dequant_output_y.texture, 1 },
      { "dctInpU", dequant_output_u.texture, 1 },
      { "dctInpV", dequant_output_v.texture, 1 },
      { NULL }
    };
    
    GLuint idct_to_rgb =
        rf_create_shader_program (vertexPassThrough, fragmentIDCTtoRGB, idct_to_rgb_unis);
    GLuint vao = rf_gen_target_buffer ();

    RFFb rgb_output = rf_make_framebuffer (GL_RGB, cpu_data->width, cpu_data->height);

    RFUniform screen_unis[] = {
      { "rgbTex", rgb_output.texture, 1 },
      { NULL }
    };

    GLuint screen_shader = rf_create_shader_program (vertexPassThrough,
        fragmentPassThrough, screen_unis);

    // rendering into the window.
    while (!glfwWindowShouldClose(window)) {

      /* zigzag --> dct Y */
      glBindFramebuffer(GL_FRAMEBUFFER, dequant_output_y.framebuffer);
      glClear(GL_COLOR_BUFFER_BIT);
      /* FIXME: group shader and unis to one structure */
      rf_use_shader_program (GL_TEXTURE_2D, dequant_shader_y, zigzag_to_dct_unis_y);
      rf_draw_to_target_buffer (vao);

//      print_line_r_tex (3, cpu_data->width, cpu_data->height);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      /* zigzag --> dct U */
      glBindFramebuffer(GL_FRAMEBUFFER, dequant_output_u.framebuffer);
      glClear(GL_COLOR_BUFFER_BIT);
      /* FIXME: group shader and unis to one structure */
      rf_use_shader_program (GL_TEXTURE_2D, dequant_shader_u, zigzag_to_dct_unis_u);
      rf_draw_to_target_buffer (vao);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      
      /* zigzag --> dct V */
      glBindFramebuffer(GL_FRAMEBUFFER, dequant_output_v.framebuffer);
      glClear(GL_COLOR_BUFFER_BIT);
      /* FIXME: group shader and unis to one structure */
      rf_use_shader_program (GL_TEXTURE_2D, dequant_shader_v, zigzag_to_dct_unis_v);
      rf_draw_to_target_buffer (vao);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      
      glBindFramebuffer(GL_FRAMEBUFFER, rgb_output.framebuffer);

      /* Apply our shader to the quad */
      rf_use_shader_program (GL_TEXTURE_2D, idct_to_rgb, idct_to_rgb_unis);
      rf_draw_to_target_buffer (vao);

      // unbind framebuffer
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      // ----------------------------------
      glClear(GL_COLOR_BUFFER_BIT);

      rf_use_shader_program(GL_TEXTURE_2D, screen_shader, screen_unis);
      rf_draw_to_target_buffer (vao);

      glViewport(0, 0, 1024, 1024);
      /* Show image on the screen */
      glfwSwapBuffers(window);

      glBindTexture(GL_TEXTURE_2D, 0);
      glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
