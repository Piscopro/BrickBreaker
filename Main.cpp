#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <vector>
#include <cmath>
#include <string>
using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int BALL_SIZE = 25;
const int BRICK_WIDTH = 70;
const int BRICK_HEIGHT = 40;
const int PADDLE_WIDTH = 125;
const int PADDLE_HEIGHT = 30;

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Texture* loadTexture(string path);

class Ball{
public:
    SDL_Rect rect;
    int direction = 1;
    SDL_Point velocity;
    bool launch = false;
    Ball(SDL_Texture* texture, int x, int y, int w, int h, float vx, float vy): texture(texture), rect({x, y, w, h}), velocity({vx, vy}){}

    void update(int paddlex, int direction){
        if(launch){
            //Ball moving around
            if(direction == 1){
                rect.x += velocity.x;
                rect.y -= velocity.y;
            }else{
                rect.x -= velocity.x;
                rect.y -= velocity.y;
            }

            //Wall collision
            if(rect.x < 0 || rect.x + rect.w > SCREEN_WIDTH) velocity.x = -velocity.x;
//            if(rect.y < 0 || rect.y + rect.h > SCREEN_HEIGHT) velocity.y = -velocity.y;
            if(rect.y < 0) velocity.y = -velocity.y;
        }else{
            rect.x = (2 * paddlex + PADDLE_WIDTH) / 2 - BALL_SIZE / 2;
        }
    }

    void render(SDL_Renderer* renderer) {
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
    }

private:
    SDL_Texture* texture;

};

class Paddle{
public:
    SDL_Rect rect;
    Paddle(SDL_Texture* texture, int x, int y, int w, int h): texture(texture), rect({x, y, w, h}) {}

    void move(int direction){
        if(direction == 1) rect.x += paddleSpeed;
        else rect.x -= paddleSpeed;

        if(rect.x < 0) rect.x = 0;
        else if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;

        if(rect.y < 0) rect.y = 0;
        else if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }

    void render(SDL_Renderer* renderer){
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
    }

private:
    SDL_Texture* texture;
    float paddleSpeed = 21.5f;
};

class Brick{
public:
    int type;
    int live;
    bool visible;
    SDL_Rect rect;
    Brick(SDL_Texture* texture, int x, int y, int w, int h): texture(texture), rect({x, y, w, h}){}

    void render(SDL_Renderer* renderer, int live){
        SDL_Rect spriteRect = clips[3 - live];
        SDL_RenderCopy(renderer, texture, &spriteRect, &rect);
    }

private:
    SDL_Texture* texture;
    SDL_Rect clips[3] = {
        { 0, 0, BRICK_WIDTH * 2, BRICK_HEIGHT * 2},
        { BRICK_WIDTH * 2, 0, BRICK_WIDTH * 2, BRICK_HEIGHT * 2 },
        { BRICK_WIDTH * 4, 0, BRICK_WIDTH * 2, BRICK_HEIGHT * 2}
    };
};

bool init(){
	bool success = 1;
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << endl;
		success = 0;
	}
	else{
		if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")){
			cout << "Warning: Linear texture filtering not enabled!";
		}
		gWindow = SDL_CreateWindow( "Brick Breaker", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if(gWindow == NULL){
			cout << "Window could not be created! SDL Error: " << SDL_GetError() << endl;
			success = 0;
		}
		else{
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if(gRenderer == NULL) {
				cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << endl;
				success = 0;
			}
			else{
				SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);

				int imgFlags = IMG_INIT_PNG;
				if(!(IMG_Init(imgFlags) & imgFlags)){
					cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << endl;
					success = 0;
				}
				if(TTF_Init() == -1){
                    cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << endl;
                    success = 0;
                }
			}
		}
	}
	return success;
}

bool loadMedia(){
    SDL_Texture* background = IMG_LoadTexture(gRenderer, "Texture/Background.png");
    SDL_Texture* ballTexture = IMG_LoadTexture(gRenderer, "Texture/Ball1.png");
    SDL_Texture* spriteBrickTexture = IMG_LoadTexture(gRenderer, "Texture/Brick Sprite.png");
    SDL_Texture* paddleTexture = IMG_LoadTexture(gRenderer, "Texture/Paddle.png");
    SDL_Texture* arrowTexture = IMG_LoadTexture(gRenderer, "Texture/Arrow.png");
}
SDL_Texture* loadTexture(string path){
	SDL_Texture* newTexture = NULL;
	SDL_Surface* loadedSurface = IMG_Load( path.c_str() );

	if(loadedSurface == NULL){
		cout << "Unable to load image" << path.c_str() << "! SDL_image Error: " << IMG_GetError() << endl;
	}
	else{
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if(newTexture == NULL){
			cout << "Unable to create texture from" << path.c_str() << "! SDL Error: " << SDL_GetError() << endl;
		}
		SDL_FreeSurface(loadedSurface);
	}
	return newTexture;
}

vector<Brick> bricks;

void handleCollisions(Ball& ball, vector<Brick>& bricks, Paddle& paddle) {
    if(SDL_HasIntersection(&ball.rect, &paddle.rect)){
        ball.velocity.y = -ball.velocity.y;
    }
    for (auto& brick : bricks) {
        if (brick.visible && SDL_HasIntersection(&ball.rect, &brick.rect)) {
            int ballCenterX = ball.rect.x + ball.rect.w / 2;
            int ballCenterY = ball.rect.y + ball.rect.h / 2;
            int brickCenterX = brick.rect.x + brick.rect.w / 2;
            int brickCenterY = brick.rect.y + brick.rect.h / 2;

            int deltaX = ballCenterX - brickCenterX;
            int deltaY = ballCenterY - brickCenterY;
            int intersectX = abs(deltaX) - (ball.rect.w + brick.rect.w) / 2;
            int intersectY = abs(deltaY) - (ball.rect.h + brick.rect.h) / 2;

            if (intersectX > intersectY) {
                // collision with top or bottom side of brick
                ball.velocity.x = -ball.velocity.x;
            } else {
                // collision with left or right side of brick
                ball.velocity.y = -ball.velocity.y;
            }
            brick.live--;
            if (brick.live == 0) {
                brick.visible = false;
            }
        }
    }
}

int main(int argc, char* argv[]){
    if(!(init())){
        cout << "Fail to initialize!";
        return -1;
    }
    loadMedia();
//    SDL_Window* window = SDL_CreateWindow("Brick Breaker", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
//    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

//    SDL_Surface* bgSurface =
    SDL_Texture* background = IMG_LoadTexture(gRenderer, "Texture/Background.png");
    SDL_Texture* ballTexture = IMG_LoadTexture(gRenderer, "Texture/Ball1.png");
    SDL_Texture* spriteBrickTexture = IMG_LoadTexture(gRenderer, "Texture/Brick Sprite.png");
    SDL_Texture* paddleTexture = IMG_LoadTexture(gRenderer, "Texture/Paddle.png");
    SDL_Texture* arrowTexture = IMG_LoadTexture(gRenderer, "Texture/Arrow.png");
    SDL_Texture* arrowLTexture = IMG_LoadTexture(gRenderer, "Texture/ArrowL.png");

    Ball ball(ballTexture, SCREEN_WIDTH / 2, SCREEN_HEIGHT - PADDLE_HEIGHT - BALL_SIZE, BALL_SIZE, BALL_SIZE, 5, 5);
    Paddle paddle(paddleTexture, SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2, SCREEN_HEIGHT - PADDLE_HEIGHT, PADDLE_WIDTH, PADDLE_HEIGHT);


    TTF_Font* font = TTF_OpenFont("arial.ttf", 64);
    SDL_Surface* surface = TTF_RenderText_Solid(font, "You Lose!", {255, 255, 255});
    SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, surface);

    SDL_Rect arrowr = {ball.rect.x + BALL_SIZE / 2, SCREEN_HEIGHT - PADDLE_HEIGHT - 30 - BALL_SIZE, 30, 30};
    SDL_Rect arrowl = {ball.rect.x, SCREEN_HEIGHT - PADDLE_HEIGHT - 30 - BALL_SIZE, -30, 30};
    SDL_Rect currentArrow = arrowr;
    SDL_Texture* currentArrowTexture = arrowTexture;

    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 6; j++){
            Brick brick(spriteBrickTexture, 175 + j * (BRICK_WIDTH + 5), 80 + i * (BRICK_HEIGHT + 5), BRICK_WIDTH, BRICK_HEIGHT);
            brick.visible = true;
            brick.live = 3;
            bricks.push_back(brick);
        }
    }

    bool quit = false;
    //Main loop
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                quit = true;
            }if(event.type == SDL_KEYDOWN){
                if(!ball.launch){
                    if(event.key.keysym.sym == SDLK_RIGHT){
                        currentArrow = arrowr;
                        currentArrowTexture = arrowTexture;
                        ball.direction = 1;
                    }else if(event.key.keysym.sym == SDLK_LEFT){
                        currentArrow = arrowl;
                        currentArrowTexture = arrowLTexture;
                        ball.direction = -1;
                    }
                    if(event.key.keysym.sym == SDLK_SPACE) ball.launch = true;

                }else{
                    if(event.key.keysym.sym == SDLK_RIGHT) paddle.move(1);
                    if(event.key.keysym.sym == SDLK_LEFT) paddle.move(-1);
                }

            }
        }

        //Render background
        SDL_RenderCopy(gRenderer, background, nullptr, nullptr);

        if(!ball.launch) SDL_RenderCopy(gRenderer, currentArrowTexture, NULL, &currentArrow);

        ball.update(paddle.rect.x, ball.direction);

        handleCollisions(ball, bricks, paddle);



        //Render bricks
        for(auto brick : bricks){
            if(brick.visible){
                brick.render(gRenderer, brick.live);
            }
        }


        //Render ball and paddle
        ball.render(gRenderer);
        paddle.render(gRenderer);

        if(ball.rect.y > SCREEN_HEIGHT){
            SDL_Rect textbox = {250, 200, 300, 80};
            SDL_RenderCopy(gRenderer, texture, NULL, &textbox);

//        quit = true;
        }
        //Update the screen
        SDL_RenderPresent(gRenderer);
    }

    SDL_DestroyTexture(background);
    SDL_DestroyTexture(ballTexture);
    SDL_DestroyTexture(spriteBrickTexture);
    SDL_DestroyTexture(paddleTexture);
    SDL_DestroyTexture(arrowTexture);
    SDL_DestroyTexture(arrowLTexture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    IMG_Quit();
    SDL_Quit();
    TTF_CloseFont(font);
    SDL_FreeSurface(surface);
    return 0;
}
