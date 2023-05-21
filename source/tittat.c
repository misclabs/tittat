#include "tittat_config.h"
#include "misc_pasta.h"
#include "raylib.h"
#include "raymath.h"
#if defined(PLATFORM_WEB)
	#include <emscripten/emscripten.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#define BgColor (Color){ 0x1a, 0x1a, 0x1a, 0xff }
#define FgColor (Color){ 0x67, 0xa9, 0xff, 0xff }
#define HlColor (Color){ 0xff, 0x5c, 0xff, 0xff }
#define DbColor (Color){ 0x76, 0xfe, 0xb2, 0xff }

#define CourtWidth 320
#define CourtHeight 200
#define BallSize 6.0f
#define BallOriginX (BallSize/2.0f)
#define BallOriginY (BallSize/2.0f)
#define BallInitialSpeed 70.0f
#define BallMaxSpeed 400.0f
#define ServeAngleRange 60
#define ScoreToWin 3
#define PlayerMaxSpeed 360.0f
#define PlayerWidth 10.0f
#define PlayerHeight 36.0f
#define PlayerOriginX (PlayerWidth/2.0f)
#define PlayerOriginY (PlayerHeight/2.0f)
#define PlayerGap 6.0f
#define PlayerBoundsTop (PlayerGap + PlayerOriginY)
#define PlayerBoundsBottom (CourtHeight - PlayerGap - PlayerOriginY)
#define PlayerDistanceFromWall 32.0f
#define FlashDuration 0.2f

typedef enum GameMode { GM_Intro, GM_BallInPlay, GM_Serving, GM_GameOver } GameMode;
typedef struct GameState {
	float courtToScreenScale;
	GameMode mode;
	Vector2 ballPos;
	Vector2 ballVel;
	Vector2 player1Pos;
	Vector2 player2Pos;
	int player1Score;
	int player2Score;
	double ballFlashTimestamp;
	double p1SectionFlashTimestamp;
	int p1FlashSection;
	double p2SectionFlashTimestamp;
	int p2FlashSection;
	double gameOverTimestamp;
} GameState;
static GameState gameState;
static Sound ballPaddleHitSound;
static Sound ballWallHitSound;
static Sound scoreSound;
static Sound winSound;
#define NPaddleSections 8
static Vector2 sectionReboundDir[NPaddleSections];

void InitGameState(GameState* state, float courtToScreenScale) {
	*state = (GameState){ 0 };
	state->courtToScreenScale = courtToScreenScale;
	state->player1Pos = (Vector2){ PlayerDistanceFromWall + PlayerOriginX / 2.0f, CourtHeight / 2.0f };
	state->player2Pos = (Vector2){ CourtWidth - (PlayerDistanceFromWall + PlayerOriginX / 2.0f), CourtHeight / 2.0f };
}

void StartGame(GameState* state) {
	state->player1Score = 0;
	state->player2Score = 0;
	state->mode = GM_Serving;
}

void Serve(GameState* state) {
	state->mode = GM_BallInPlay;
	
	float angle = (float)(rand() % (ServeAngleRange*2));
	if (angle >= ServeAngleRange) {
		angle = angle + (180.0f - (ServeAngleRange * 1.5f));
	} else {
		angle = angle - (ServeAngleRange / 2.0f);
	}
	angle = angle * DEG2RAD;

	state->ballPos = (Vector2){ CourtWidth / 2.0f, CourtHeight / 2.0f };
	state->ballVel = Vector2Scale((Vector2) { (float)cos(angle), (float)sin(angle) }, BallInitialSpeed);
}

float CalcNewSpeed(float speed) {
	assert(speed != 0.0f);
	if (speed == 0.0f)
		speed = 0.1f;

	return speed * 1.075f > BallMaxSpeed ? BallMaxSpeed : speed * 1.075f;
}

int DetectBallPlayerCollisionSection(Vector2 ballPos, Vector2 playerPos) {
	if (!(ballPos.x + BallSize/2.0f >= playerPos.x - PlayerOriginX
		&& ballPos.x - BallSize/2.0f <= playerPos.x + PlayerOriginX
		&& ballPos.y + BallSize/2.0f >= playerPos.y - PlayerOriginY
		&& ballPos.y - BallSize/2.0f <= playerPos.y + PlayerOriginY))
		return -1;

	float verticlePenetration = ballPos.y - (playerPos.y - PlayerOriginY);
	float sectionLength = PlayerHeight / NPaddleSections;
	int section = (int)floorf(verticlePenetration / sectionLength);
	
	return MpClampI(section, 0, NPaddleSections - 1);
}

void DrawPlayer(Vector2 pos, float scale, Color color, int flashSection) {
	Rectangle drawRect = {
		pos.x * scale, pos.y * scale,
		PlayerWidth * scale, PlayerHeight * scale
	};
	Vector2 origin = { PlayerOriginX * scale, PlayerOriginY * scale };
	DrawRectanglePro(drawRect, origin, 0, color);

	if (flashSection >= 0) {
		float sectionHeight = PlayerHeight / NPaddleSections;
		drawRect.y = (pos.y + sectionHeight * flashSection) * scale;
		drawRect.height = PlayerHeight / NPaddleSections * scale;
		DrawRectanglePro(drawRect, origin, 0, HlColor);
	}
}
void DrawTextPivot(const char* text, Vector2 pos, Vector2 pivot, float fontSize) {
	assert(fontSize >= 10.0f);
	float spacing = fontSize / 10.0f;
	Font font = GetFontDefault();
	Vector2 metrics = MeasureTextEx(font, text, fontSize, spacing);
	pos.x = pos.x - metrics.x * pivot.x;
	pos.y = pos.y - metrics.y * pivot.y;
	DrawTextEx(font, text, pos, fontSize, spacing, FgColor);
}

void DrawScore(int leftScore, int rightScore, float fontSize, float inset) {
	DrawTextPivot(TextFormat("%i", leftScore), 
		(Vector2){ inset, 16 }, 
		(Vector2){ 0, 0 }, fontSize);
	DrawTextPivot(TextFormat("%i", rightScore), 
		(Vector2){ GetScreenWidth() - inset, 16 }, 
		(Vector2){ 1.0f, 0 }, fontSize);
}

void Tick(void* userData) {
	GameState* state = (GameState*)userData;
	const double time = GetTime();
	const float dt = GetFrameTime();

	// Collect paddle input
	float p1Input = 0.0f;
	float p2Input = 0.0f;
	{
		if (IsKeyDown(KEY_W))
			p1Input -= 1.0f;
		if (IsKeyDown(KEY_S))
			p1Input += 1.0f;
		if (IsKeyDown(KEY_UP))
			p2Input -= 1.0f;
		if (IsKeyDown(KEY_DOWN))
			p2Input += 1.0f;

		state->player1Pos.y = MpClampF(
			state->player1Pos.y + p1Input * PlayerMaxSpeed * dt, 
			PlayerBoundsTop, PlayerBoundsBottom);
		state->player2Pos.y = MpClampF(
			state->player2Pos.y + p2Input * PlayerMaxSpeed * dt, 
			PlayerBoundsTop, PlayerBoundsBottom);
	}

	// Update
	if (state->mode == GM_Intro) {
		if (IsKeyPressed(KEY_SPACE))
			StartGame(state);
	} else if (state->mode == GM_GameOver){
		if (IsKeyPressed(KEY_SPACE))
			StartGame(state);
		else if (IsKeyPressed(KEY_H))
			state->mode = GM_Intro;
	} else if (state->mode == GM_Serving) {
		if (IsKeyPressed(KEY_SPACE))
			Serve(state);
	} else if (state->mode == GM_BallInPlay) {
		state->ballPos = Vector2Add(state->ballPos, Vector2Scale(state->ballVel, dt));

		// Check scoring bounds collisions
		bool touchedLeft = state->ballPos.x + BallOriginX <= 0.0f;
		if (touchedLeft || state->ballPos.x - BallOriginX >= CourtWidth) {
			if (touchedLeft)
				state->player2Score += 1;
			else
				state->player1Score += 1;

			if (state->player1Score == ScoreToWin || state->player2Score == ScoreToWin) {
				PlaySound(winSound);
				state->mode = GM_GameOver;
				state->gameOverTimestamp = GetTime();
			} else {
				PlaySound(scoreSound);
				state->mode = GM_Serving;
			}
		}

		// Check floor and ceiling collisions
		if ((state->ballPos.y - BallOriginY <= 0.0f && state->ballVel.y < 0.0f) 
			|| (state->ballPos.y + BallOriginY >= CourtHeight && state->ballVel.y > 0.0f)) 
		{
			state->ballVel.y = -state->ballVel.y;
			PlaySound(ballWallHitSound);
		}

		int p1HitSection = DetectBallPlayerCollisionSection(state->ballPos, state->player1Pos);
		if (p1HitSection != -1 && state->ballVel.x < 0.0f) {
			state->ballFlashTimestamp = time;
			state->p1SectionFlashTimestamp = time;
			state->p1FlashSection = p1HitSection;

			float speed = Vector2Length(state->ballVel);
			float newSpeed = CalcNewSpeed(speed);
			state->ballVel = Vector2Scale(sectionReboundDir[p1HitSection], newSpeed);

			PlaySound(ballPaddleHitSound);
		}
		int p2HitSection = DetectBallPlayerCollisionSection(state->ballPos, state->player2Pos);
		if (p2HitSection != -1  && state->ballVel.x > 0.0f) {
			state->ballFlashTimestamp = time;
			state->p2SectionFlashTimestamp = time;
			state->p2FlashSection = p2HitSection;

			float speed = Vector2Length(state->ballVel);
			float newSpeed = CalcNewSpeed(speed);
			state->ballVel = Vector2Scale(sectionReboundDir[p2HitSection], newSpeed);
			state->ballVel.x = -state->ballVel.x;

			PlaySound(ballPaddleHitSound);
		}
	}

	BeginDrawing(); {
		const float drawScale = state->courtToScreenScale;
		const int screenWidth = GetScreenWidth();
		const int screenHeight = GetScreenHeight();
		const float BigFontSize = 32.0f * state->courtToScreenScale;
		const float NormalFontSize = 10.0f * state->courtToScreenScale;

		ClearBackground(FgColor);
		DrawRectangle(3, 3, screenWidth-6, screenHeight-6, BgColor);

		DrawPlayer(state->player1Pos, drawScale, FgColor, time - state->p1SectionFlashTimestamp < FlashDuration ? state->p1FlashSection : -1);
		DrawPlayer(state->player2Pos, drawScale, FgColor, time - state->p2SectionFlashTimestamp < FlashDuration ? state->p2FlashSection : -1);

		if (state->mode == GM_BallInPlay) {
			DrawScore(state->player1Score, state->player2Score, 
				BigFontSize, state->player1Pos.x * 2 * state->courtToScreenScale);

			/* DrawBall */ {
				Color ballColor = time - state->ballFlashTimestamp < FlashDuration ? HlColor : FgColor;
				DrawRectanglePro(
					(Rectangle) {
					state->ballPos.x* drawScale, state->ballPos.y* drawScale,
						BallSize* drawScale, BallSize* drawScale },
					(Vector2) { BallOriginX* drawScale, BallOriginY* drawScale },
						0, ballColor);
			}
		}  else if (state->mode == GM_Serving) {
			DrawScore(state->player1Score, state->player2Score, 
				BigFontSize, state->player1Pos.x * 2 * state->courtToScreenScale);

			DrawTextPivot("[Space] to serve", (Vector2){screenWidth/2.0f, screenHeight/2.0f}, (Vector2){0.5f, 0.5f}, NormalFontSize);
		} else if (state->mode == GM_GameOver) {
			DrawScore(state->player1Score, state->player2Score, 
				BigFontSize, state->player1Pos.x * 2 * state->courtToScreenScale);

			const char* winnerText;
			if (state->player1Score > state->player2Score)
				winnerText = "Left player won!";
			else
				winnerText = "Right player won!";

			DrawTextPivot(winnerText,
				(Vector2){screenWidth/2.0f, screenHeight/2.0f}, 
				(Vector2){0.5f, 1.0f}, NormalFontSize);	
			DrawTextPivot("[Space] to play again\n[h] for instructions",
				(Vector2){ screenWidth/2.0f, screenHeight/2.0f },
				(Vector2) { 0.5f, -1.0f }, NormalFontSize);
		} else if (state->mode == GM_Intro) {
			Vector2 titlePos = { screenWidth / 2.0f, screenHeight / 4.0f };
			DrawTextPivot("How about a game of", titlePos, (Vector2) { 0.8f, 1.5f }, NormalFontSize);
			DrawTextPivot("TitTat?", titlePos, (Vector2) { 0.5f, 0.0f }, BigFontSize);

			DrawTextPivot("[Space] to play",
				(Vector2){ screenWidth/2.0f, screenHeight/2.0f },
				(Vector2) { 0.5f, -1.0f }, NormalFontSize);
			
			float instructionsY = screenHeight / 3.0f * 2.0f;
			DrawTextPivot("Left Player Keys:\n[W] Move up\n[S] Move down",
				(Vector2){ screenWidth/2.0f - NormalFontSize*3.0f, instructionsY },
				(Vector2) { 1.0f, 0.0f }, NormalFontSize);

			DrawTextPivot("Right Player Keys:\n[Up arrow] Move up\n[Down arrow] move down",
				(Vector2){ screenWidth/2.0f + NormalFontSize*3.0f, instructionsY },
				(Vector2) { 0.0f, 0.0f }, NormalFontSize);
		}
	} EndDrawing();
}

int main(void) {

	const float courtToScreenScale = 3.0f;

	TraceLog(LOG_INFO, "Tittat version %d.%d", TittatVersionMajor, TittatVersionMinor);
	
	{ // Initialize upstream
		srand((unsigned int)time(NULL));
		InitWindow((int)(CourtWidth * courtToScreenScale), (int)(CourtHeight * courtToScreenScale), "TitTat");
		InitAudioDevice();
	}

	{ // Load
		ballPaddleHitSound = LoadSound("content/ball_paddle_hit.wav");
		ballWallHitSound = LoadSound("content/ball_wall_hit.wav");
		scoreSound = LoadSound("content/score.wav");
		winSound = LoadSound("content/win.wav");
	}

	{
		float fullArcAngle = PI * 2.0f / 4.0f;
		float sectionArcAngle = fullArcAngle / (NPaddleSections-1);
		for (int i = 0; i < NPaddleSections; ++i) {
			float dirRad = sectionArcAngle * i - fullArcAngle / 2.0f;
			sectionReboundDir[i] = (Vector2) { cosf(dirRad), sinf(dirRad) };
		}
	}

	InitGameState(&gameState, courtToScreenScale);

#if defined(PLATFORM_WEB)
	emscripten_set_main_loop_arg(Tick, &gameState, 0, 1);
#endif

#if defined(PLATFORM_DESKTOP) && !defined(PLATFORM_WEB)
	SetWindowState(FLAG_VSYNC_HINT);
#elif !defined(PLATFORM_WEB)
	SetTargetFPS(60);
#endif

#if !defined(PLATFORM_WEB)
	while (!WindowShouldClose()) {
		Tick(&gameState);
	}

	UnloadSound(ballPaddleHitSound);
	UnloadSound(ballWallHitSound);
	UnloadSound(scoreSound);
	CloseAudioDevice();
	CloseWindow();
#endif

	return 0;
}
