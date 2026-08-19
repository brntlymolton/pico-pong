#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"


// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define ADC_PIN 26 // GPIO26 is used for the potentiometer input

enum GameState {
    WAITING,
    PLAYING,
    PLAYER_WON,
    BOT_WON,
};

GameState state = WAITING;


void draw_hud(ssd1306_t *display, char *pScore_str, char *bScore_str)
{
    // Draw the HUD elements on the display
    ssd1306_draw_line(display, 0, 10, 128, 10); // Draw score line
    ssd1306_draw_string(display, 0, 0, 1, pScore_str); // Draw player score
    ssd1306_draw_string(display, 122, 0, 1, bScore_str); // Draw bot score
    ssd1306_draw_string(display, 52, 0, 1, "PONG"); // Draw game title
}

// Map one range onto another
int map(int value,
        int in_min, int in_max,
        int out_min, int out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Check win condition
void win(int ps, int bs) {
    if (ps == 7) {
        state = PLAYER_WON;
    }
    if (bs == 7) {
        state = BOT_WON;
    }
}


struct Paddle {
    int x; int y; int width; int height;

    void draw(ssd1306_t *display) {
        ssd1306_draw_square(display, x, y, width, height);
    }
};

struct Ball {
    int x; int y; int width; int height; int dx; int dy;
    int timer = 0;
    int moveframe = 0;

    void draw(ssd1306_t *display) {
        ssd1306_draw_square(display, x, y, width, height);
    }

    void update() {
        if (state == WAITING) {
            timer++;
            
            if (timer > 60) { // If there has been 120 frames, 
                state = PLAYING; // ...Let the game play,
            }

            return; // ...Otherwise stop the rest of this method running

        }

        x += dx;
        y += dy;
    }

    void collision(Paddle& p) {
        // Handle paddle collisions
        if (y+height >= p.y &&
            y <= p.y + p.height &&
            x <= p.x + p.width &&
            x + width >= p.x) 
            {
                dx = -dx; // Ball hit paddle, reverse horizontal direction

                if (y + 1 < p.y + 4) { // If ball hits top
                    dy += 1; // Slightly increase vertical upward speed
                }
                else if (y + 1 > p.y + 8) { // Ball hits bottom
                    dy -= 1; // Slightly increase vertical downward speed
                }
            }
    }

    void wallCheck() {
        // Handle wall collisions
        if (y <= 12) { // Hits top
            dy = -dy;
            y = 13;
        }
        if (y >= 62) { // Hits bottom
            dy = -dy;
            y = 61;
        }
    }

    bool goalCheck(int& ps, int& bs) {
        if (x + width <= -8) { // Off the left side, bot scores
            bs++;
            return true;
        }
        if (x >= 136) { // Off the right side, player scores
            ps++;
            return true;
        }
        return false; // Ball doesn't go offscreen that frame
    }

    void reset() {
        x = 63;
        y = 41;
        dx = -dx; // Ball goes to winner's side
        if (get_rand_32() % 2 == 0) { dy = 2; } // Ball randomly goes up or down
        else { dy = -2;}

        timer = 0; // Reset wait timer
        state = WAITING; // Momentarily pause game
    }

    void botai(Paddle& p) {
        moveframe++;

        if (dx > 0) {
            if (y + 1 < p.y + 5 && p.y > 13) { // Move up
                if (moveframe % 2 == 0) { p.y--; p.y--; }
                else { p.y--; }
            }

            if (y + 1 > p.y + 8 && p.y < 52) { // Move down
                if (moveframe % 2 == 0) { p.y++; p.y++; }
                else { p.y++; }
            }
        }
    }
};


int main()
{
    stdio_init_all();
    sleep_ms(2000); // Wait for USB Serial to be ready

    // Pot init
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);

    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set GPIO8 as I2C SDA
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // GPIO9 as I2C SCL
    gpio_pull_up(I2C_SDA); // pull-ups are required for I2C
    gpio_pull_up(I2C_SCL); // usually built into oled, but doesn't hurt

    ssd1306_t display = {}; // Create a display object

    ssd1306_init( // Initialize the display
        &display,
        128, // width
        64,  // height
        0x3C, // i2c address
        I2C_PORT // i2c instance
    );
    sleep_ms(100); // Wait for the display to initialize

    ssd1306_clear(&display); // Clear the screen
    ssd1306_draw_square(&display, 0, 0, 128, 64); // Fill screen
    ssd1306_show(&display);
    sleep_ms(200);

    bool gameStarted = false;

    int pScore = 0;
    int bScore = 0;
    char pScore_str[8];
    char bScore_str[8];

    Ball ball = {63, 41, 2, 2, 3, 2}; // Ball object
    Paddle player = {2, 13, 2, 12}; // Player paddle
    Paddle bot = {124, 13, 2, 12}; // Bot paddle 


    while (true) {
        snprintf(pScore_str, sizeof(pScore_str), "%d", pScore); // Convert score into usable string
        snprintf(bScore_str, sizeof(bScore_str), "%d", bScore);

        
        ssd1306_clear(&display); // Clear the screen


        draw_hud(&display, pScore_str, bScore_str); // Draw the HUD


        player.y = map(adc_read(), 0, 4095, 13, 52); // Map pot value to paddle position
        player.draw(&display); // Draw player paddle

        //bot.y = map(adc_read(), 0, 4095, 13, 52);
        bot.draw(&display); // Draw bot paddle

        ball.update(); // Update ball position        
        ball.collision(player); // Check for collision
        ball.collision(bot);
        ball.wallCheck();
        if (ball.goalCheck(pScore, bScore)) { ball.reset(); } // Check if someone scores
        ball.botai(bot);

        ball.draw(&display); // Draw the ball


        ssd1306_show(&display); // Show the display


        win(pScore, bScore);
        if (state == PLAYER_WON) {
            ssd1306_clear(&display);
            draw_hud(&display, pScore_str, bScore_str);
            ssd1306_draw_string(&display, 6, 30, 2, "PLAYER WIN");
            ssd1306_show(&display);
            while (true) {}
        }
        if (state == BOT_WON) {
            ssd1306_clear(&display);
            draw_hud(&display, pScore_str, bScore_str);
            ssd1306_draw_string(&display, 24, 30, 2, "BOT WIN");
            ssd1306_show(&display);
            while (true) {}
        }

        sleep_ms(8); // Delay to reach ~120 FPS

    }
}