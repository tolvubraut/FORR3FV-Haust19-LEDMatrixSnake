// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>

#define UP    +2
#define DOWN  -2
#define RIGHT +1
#define LEFT  -1
#define WHITE 255, 255, 255

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

struct Segment {
    int x;
    int y;
    Segment() : x(0), y(0) {}
    Segment(int x, int y) : x(x), y(y) {}
    void move(int dir, int amount) {
        if (dir == UP) {
            this->y -= amount;
        }
        else if (dir == DOWN) {
            this->y += amount;
        }
        else if (dir == LEFT) {
            this->x -= amount;
        }
        else {
            this->x += amount;
        }
    }
};

static void DrawSegment(Canvas *canvas, int segment_size, int posX, int posY) {
    for (unsigned int j = 0; j < segment_size; j++) {
        for (unsigned int k = 0; k < segment_size; k++) {
            canvas->SetPixel(posX + j, posY + k, WHITE);
        }
    }
}

static void DrawGame(Canvas *canvas) {
    /*
     * Let's create a simple animation. We use the canvas to draw
     * pixels. We wait between each step to have a slower animation.
     */
    canvas->Fill(0, 0, 0);

    int center_x = canvas->width() / 2;
    int center_y = canvas->height() / 2;
    unsigned int segment_size = 2;
    unsigned int snakeWidth = 1;
    unsigned int maxSnakeWidth = 5;

    int direction = RIGHT;

    Segment snakeTrail[maxSnakeWidth];

    for (int i = 0; i < maxSnakeWidth; i++) {
        snakeTrail[i].move(LEFT, i);
    }

    bool snakeIsDead = false;

    while (!snakeIsDead) {
        if (interrupt_received)
            return;

        // For every snake segment, move it according to move values
        for (unsigned int i = 0; i < maxSnakeWidth; i++) {
            // Calculate pixel positions (origin is bottom left) for each segment
            int posX = center_x + snakeTrail[i].x * segment_size;
            int posY = center_y + snakeTrail[i].y * segment_size;
            // Draw each segment
            DrawSegment(canvas, segment_size, posX, posY);

            // TODO: Move only according to direction, then make other segments (index 1 and above) follow previous segment
            snakeTrail[i].move(direction, 1);
        }

        usleep(1000 * 1000);
        canvas->Clear();
    }
}

int main(int argc, char *argv[]) {
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
    defaults.rows = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.show_refresh_rate = true;
    Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults);
    if (canvas == NULL)
        return 1;

    // It is always good to set up a signal handler to cleanly exit when we
    // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
    // for that.
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    DrawGame(canvas);    // Using the canvas.

    // Animation finished. Shut down the RGB matrix.
    canvas->Clear();
    delete canvas;

    return 0;
}
