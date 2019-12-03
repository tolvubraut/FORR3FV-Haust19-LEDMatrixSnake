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
#include <iostream>
#include <chrono>
#include <future>

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
    bool collidesWith(Segment other) {
        return this->x == other.x && this->y == other.y;
    }
};

static void DrawSegment(Canvas *canvas, unsigned int segment_size, int posX, int posY) {
    for (unsigned int j = 0; j < segment_size; j++) {
        for (unsigned int k = 0; k < segment_size; k++) {
            canvas->SetPixel(posX + j, posY + k, WHITE);
        }
    }
}

// Get user input from keyboard
static std::string getInput() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// Set snake direction, do not allow going in the opposite direction
static void setDirection(int &direction, int newDirection) {
    if ((newDirection == UP && direction == DOWN) ||
        (newDirection == DOWN && direction == UP) ||
        (newDirection == LEFT && direction == RIGHT) ||
        (newDirection == RIGHT && direction == LEFT)) {
        return;
    }
    direction = newDirection;
}

static void GameLoop(Canvas *canvas) {
    canvas->Fill(0, 0, 0);

    int center_x = canvas->width() / 2;
    int center_y = canvas->height() / 2;
    unsigned int segment_size = 2;
    unsigned int snakeWidth = 1;
    unsigned int maxSnakeWidth = 5;

    int direction = RIGHT;

    // Initialize snake
    Segment snakeTrail[maxSnakeWidth];
    for (int i = 0; i < maxSnakeWidth; i++) {
        snakeTrail[i].move(LEFT, i);
    }

    bool snakeIsDead = false;

    auto future = std::async(std::launch::async, getInput);

    while (!snakeIsDead) {
        if (interrupt_received)
            return;

        // Get user input asynchronously (non-blocking)
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto input = future.get();
            if (input == "w") setDirection(direction, UP);
            else if (input == "a") setDirection(direction, LEFT);
            else if (input == "s") setDirection(direction, DOWN);
            else if (input == "d") setDirection(direction, RIGHT);
            future = std::async(std::launch::async, getInput);
        }

        // Move snake segments according to head position
        for (unsigned int i = maxSnakeWidth-1; i >= 1; i--) {
            snakeTrail[i].x = snakeTrail[i-1].x;
            snakeTrail[i].y = snakeTrail[i-1].y;
        }
        // Move head in desired direction
        snakeTrail[0].move(direction, 1);

        // Draw every segment
        for (unsigned int i = 0; i < maxSnakeWidth; i++) {
            // Snake dies if head collides with any of its segments
            if (i > 0 && snakeTrail[0].collidesWith(snakeTrail[i])) {
                snakeIsDead = true;
                break;
            }
            // Calculate pixel positions (origin is bottom left) for each segment
            int posX = center_x + snakeTrail[i].x * segment_size;
            int posY = center_y + snakeTrail[i].y * segment_size;
            // Draw each segment
            DrawSegment(canvas, segment_size, posX, posY);
        }

        // Check for collisions
        // If head goes off screen, snake dies
        if (center_x + snakeTrail[0].x * segment_size >= canvas->width() - segment_size || 
            center_x + snakeTrail[0].x * segment_size <= segment_size ||
            center_y + snakeTrail[0].y * segment_size >= canvas->height() - segment_size ||
            center_y + snakeTrail[0].y * segment_size <= segment_size) {
            snakeIsDead = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 0.5s delay
        canvas->Clear();
    }
}

int main(int argc, char *argv[]) {
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
    defaults.rows = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.show_refresh_rate = false;
    Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults);
    if (canvas == NULL)
        return 1;

    // It is always good to set up a signal handler to cleanly exit when we
    // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
    // for that.
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    GameLoop(canvas);    // Using the canvas.

    // Animation finished. Shut down the RGB matrix.
    canvas->Clear();
    delete canvas;

    return 0;
}
