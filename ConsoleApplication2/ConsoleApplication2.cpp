#include "stdafx.h"
#include <sstream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "player.h"
#include "ZombieArena.h"
#include "TextureHolder.h"
#include "Bullet.h"
#include "Pickup.h"
using namespace sf;
int main() {
	TextureHolder holder;
	enum class State {
		PAUSED, LEVELING_UP, GAME_OVER, PLAYING
	};
	State state = State::GAME_OVER;

	sf::Vector2f resolution;
	resolution.x = sf::VideoMode::getDesktopMode().width;
	resolution.y = sf::VideoMode::getDesktopMode().height;
	resolution.x = 1920;
	resolution.y = 1080;

	sf::RenderWindow window( sf::VideoMode(resolution.x,resolution.y),
		"Zombie Arena",sf::Style::Fullscreen );

	sf::View mainView( sf::FloatRect(0,0,resolution.x,resolution.y) ) ;
	
	mainView.zoom(0.5f);
	sf::Clock clock;

	sf::Time gameTimeTotal{};

	sf::Vector2f mouseWorldPosition;

	sf::Vector2i mouseScreenPosition;

	Player player;

	sf::IntRect arena;

	sf::VertexArray background;
	
	sf::Texture textureBackground = TextureHolder::getTexture("graphics/background_sheet.png");

	int numZombies;
	int numZombiesAlive;
	Zombie* zombies = nullptr;

	Bullet bullets[100];
	int currentBullet = 0;
	int bulletsSpare = 240;
	int bulletsInClip = 10;
	int clipSize = 6;
	float fireRate = 4;

	Time lastPressed;

	//Tolgo il cursore del mouse e metto il mirino
	window.setMouseCursorVisible(false);
	Sprite spriteCrosshair;
	spriteCrosshair.setTexture(TextureHolder::getTexture("graphics/crosshair.png"));
	spriteCrosshair.setOrigin(25, 25);

	int score = 0; 
	int hiScore = 0;

	Pickup healthPickup(1);
	Pickup ammoPickup(2);

	// For the home/game over screen
	Sprite spriteGameOver;
	Texture textureGameOver = TextureHolder::getTexture("graphics/background.png");
	spriteGameOver.setTexture(textureGameOver);
	spriteGameOver.setPosition(0, 0);

	// Create a view for the HUD
	View hudView(sf::FloatRect(0, 0, resolution.x, resolution.y));

	// Create a sprite for the ammo icon
	Sprite spriteAmmoIcon;
	Texture textureAmmoIcon = TextureHolder::getTexture("graphics/ammo_icon.png");
	spriteAmmoIcon.setTexture(textureAmmoIcon);
	spriteAmmoIcon.setPosition(20, hudView.getSize().y - 200);

	// Load the font
	Font font;
	font.loadFromFile("fonts/zombiecontrol.ttf");

	// Paused
	Text pausedText;
	pausedText.setFont(font);
	pausedText.setCharacterSize(155);
	pausedText.setFillColor(Color::White);
	pausedText.setPosition(400, 400);
	pausedText.setString("Press Enter \nto continue");

	// Game Over
	Text gameOverText;
	gameOverText.setFont(font);
	gameOverText.setCharacterSize(125);
	gameOverText.setFillColor(Color::White);
	gameOverText.setPosition(250, 850);
	gameOverText.setString("Press Enter to play");

	// Levelling up
	Text levelUpText;
	levelUpText.setFont(font);
	levelUpText.setCharacterSize(80);
	levelUpText.setFillColor(Color::White);
	levelUpText.setPosition(150, 250);
	std::stringstream levelUpStream;
	levelUpStream <<
		"1- Increased rate of fire" <<
		"\n2- Increased clip size(next reload)" <<
		"\n3- Increased max health" <<
		"\n4- Increased run speed" <<
		"\n5- More and better health pickups" <<
		"\n6- More and better ammo pickups";
	levelUpText.setString(levelUpStream.str());

	// Ammo
	Text ammoText;
	ammoText.setFont(font);
	ammoText.setCharacterSize(55);
	ammoText.setFillColor(Color::White);
	ammoText.setPosition(200, hudView.getSize().y-200);

	// Score
	Text scoreText;
	scoreText.setFont(font);
	scoreText.setCharacterSize(55);
	scoreText.setFillColor(Color::White);
	scoreText.setPosition(20, 20);

	std::ifstream inputFile("gameData/scores.txt");
	if (inputFile.is_open()) {
		inputFile >> hiScore;
		inputFile.close();
	}

	// Hi Score
	Text hiScoreText;
	hiScoreText.setFont(font);
	hiScoreText.setCharacterSize(55);
	hiScoreText.setFillColor(Color::White);
	hiScoreText.setPosition(hudView.getSize().x - 300, 20);
	std::stringstream s;
	s << "Hi Score:" << hiScore;
	hiScoreText.setString(s.str());

	// Zombies remaining
	Text zombiesRemainingText;
	zombiesRemainingText.setFont(font);
	zombiesRemainingText.setCharacterSize(55);
	zombiesRemainingText.setFillColor(Color::White);
	zombiesRemainingText.setPosition(1200, hudView.getSize().y - 200);
	zombiesRemainingText.setString("Zombies: 100");

	// Wave number
	int wave = 0;
	Text waveNumberText;
	waveNumberText.setFont(font);
	waveNumberText.setCharacterSize(55);
	waveNumberText.setFillColor(Color::White);
	waveNumberText.setPosition(hudView.getSize().x -300, hudView.getSize().y - 200);
	waveNumberText.setString("Wave: 0");

	// Health bar
	RectangleShape healthBar;
	healthBar.setFillColor(Color::Red);
	healthBar.setPosition(450, hudView.getSize().y - 200);

	// When did we last update the HUD?
	int framesSinceLastHUDUpdate = 0;

	// How often (in frames) should we update the HUD
	int fpsMeasurementFrameInterval = 1000;

	// Prepare the hit sound
	SoundBuffer hitBuffer;
	hitBuffer.loadFromFile("sound/hit.wav");
	Sound hit;
	hit.setBuffer(hitBuffer);

	// Prepare the splat sound
	SoundBuffer splatBuffer;
	splatBuffer.loadFromFile("sound/splat.wav");
	sf::Sound splat;
	splat.setBuffer(splatBuffer);

	// Prepare the shoot sound
	SoundBuffer shootBuffer;
	shootBuffer.loadFromFile("sound/shoot.wav");
	Sound shoot;
	shoot.setBuffer(shootBuffer);

	// Prepare the reload sound
	SoundBuffer reloadBuffer;
	reloadBuffer.loadFromFile("sound/reload.wav");
	Sound reload;
	reload.setBuffer(reloadBuffer);

	// Prepare the failed sound
	SoundBuffer reloadFailedBuffer;
	reloadFailedBuffer.loadFromFile("sound/reload_failed.wav");
	Sound reloadFailed;
	reloadFailed.setBuffer(reloadFailedBuffer);

	// Prepare the powerup sound
	SoundBuffer powerupBuffer;
	powerupBuffer.loadFromFile("sound/powerup.wav");
	Sound powerup;
	powerup.setBuffer(powerupBuffer);

	// Prepare the pickup sound
	SoundBuffer pickupBuffer;
	pickupBuffer.loadFromFile("sound/pickup.wav");
	Sound pickup;
	pickup.setBuffer(pickupBuffer);

	//main loop
	while (window.isOpen()) {

		sf::Event event;

		// Event polling
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Return && state == State::PLAYING) {
					state = State::PAUSED;
				}
				else if (event.key.code == sf::Keyboard::Return && state == State::PAUSED) {
					state = State::PLAYING;
					clock.restart(); //L'orologio viene ristartato perchè mentre il gioco è in pausa lui continua a conteggiare e quando poi vado a restartare è come se il frame sia durato un sacco di tempo
				}
				else if (event.key.code == sf::Keyboard::Return && state == State::GAME_OVER) {
					state = State::LEVELING_UP;
					wave = 0;
					score = 0;
					currentBullet = 0;
					bulletsSpare = 2400;
					bulletsInClip = 10;
					clipSize = 10;
					fireRate = 5;

					player.resetPlayerStats();
				}

				if (state == State::PLAYING) {
					if (event.key.code == Keyboard::R) {
						if (bulletsSpare >= clipSize) {
							bulletsInClip = clipSize;
							bulletsSpare -= clipSize;
							reload.play();
						}
						else if(bulletsSpare > 0){
							bulletsInClip = bulletsSpare;
							bulletsSpare = 0;
							reload.play();
						}
						else {
							reloadFailed.play();
						}
					}
				}

			}
		} // End event polling

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
			window.close();
		}

		//WASD Player Movement
		if (state == State::PLAYING) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
				player.moveUp();
			}
			else {
				player.stopUp();
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
				player.moveDown();
			}
			else {
				player.stopDown();
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
				player.moveLeft();
			}
			else {
				player.stopLeft();
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
				player.moveRight();
			}
			else {
				player.stopRight();
			}

			//Fire a bullet
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				if (gameTimeTotal.asMilliseconds() - lastPressed.asMilliseconds()
					> 1000 / fireRate && bulletsInClip > 0) {
					
					bullets[currentBullet].shoot(player.getCenter().x, player.getCenter().y, mouseWorldPosition.x, mouseWorldPosition.y);
					currentBullet++;
					if (currentBullet > 99) {
						currentBullet = 0;
					}
					lastPressed = gameTimeTotal;
					bulletsInClip--;
					shoot.play();
				}
			}

		}//End WASD Handling

		 //Handle Levelling up
		if (state == State::LEVELING_UP) {
			if (event.key.code == sf::Keyboard::Num1) {
				//Increase fire rate
				fireRate++;
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num2) {
				//Increase clipSize
				clipSize+=2;
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num3) {
				//Increase Health
				player.upgradeHealth();
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num4) {
				//Increase Speed
				player.upgradeSpeed();
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num5) {
				//Upgrade Pickup
				healthPickup.upgrade();
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num6) {
				ammoPickup.upgrade();
				state = State::PLAYING;
			}

			if (state == State::PLAYING) {

				wave++;
				if (wave == 1) {
					arena.width = 800 * wave;
					arena.height = 800 * wave;
				}
				else {
					arena.width = 500 * wave;
					arena.height = 500 * wave;
				}

				
				arena.left = 0;
				arena.top = 0;
				int tileSize = createBackground(background,arena);
				player.spawn(arena, resolution, tileSize);
				healthPickup.setArena(arena);
				ammoPickup.setArena(arena);

				numZombies = 7 * wave;
				delete[] zombies;
				zombies = createHorde(numZombies, arena);
				numZombiesAlive = numZombies;

				powerup.play();

				clock.restart();
			}
		}

		//UPDATE
		if (state == State::PLAYING) {
			sf::Time dt = clock.restart();
			gameTimeTotal += dt;
			float dtAsSeconds = dt.asSeconds();
			mouseScreenPosition = sf::Mouse::getPosition();

			mouseWorldPosition = window.mapPixelToCoords(
				sf::Mouse::getPosition(), mainView);

			spriteCrosshair.setPosition(mouseWorldPosition);

			player.update(dtAsSeconds, sf::Mouse::getPosition());

			sf::Vector2f playerPosition(player.getCenter());
			mainView.setCenter(player.getCenter());

			for (int i = 0; i < numZombies; i++) {
				if (zombies[i].isAlive()) {
					zombies[i].update(dt.asSeconds(), playerPosition);
				}
			}

			for (int i = 0; i < 100; i++) {
				if (bullets[i].isInFlight()) {
					bullets[i].update(dtAsSeconds);
				}
			}

			healthPickup.update(dtAsSeconds);
			ammoPickup.update(dtAsSeconds);

			//Collisione proiettili/zombie
			for (int i = 0; i < 100; i++) {
				if (bullets[i].isInFlight()) {
					for (int j = 0; j < numZombies; j++) {
						if (zombies[j].isAlive()) {
							if (bullets[i].getPosition().intersects(zombies[j].getPosition())) {
								bullets[i].stop();
								if (zombies[j].hit()) {
									score += 10;
									if (score > hiScore) {
										hiScore = score;
									}
									numZombiesAlive--;
									if (numZombiesAlive == 0) {
										state = State::LEVELING_UP;
									}
								}
								splat.play();
							}
						}
					}
				}
			}

			//Gestione collisione zombie/giocatore
			for (int i = 0; i < numZombies; i++) {
				if (zombies[i].isAlive() && player.getPosition().intersects(zombies[i].getPosition())) {
					if (player.hit(gameTimeTotal)) {
						hit.play();
					}
					if (player.getHealth() <= 0) {
						state = State::GAME_OVER;
						std::ofstream outputFile("gamedata/scores.txt");
						outputFile << hiScore;
						outputFile.close();
					}
				}
			}

			//Gestione collisione pickup/giocatore
			if (healthPickup.isSpawned() && player.getPosition().intersects(healthPickup.getPosition()))
			{
				player.increaseHealthLevel(healthPickup.gotIt());
				pickup.play();
			}
			if (ammoPickup.isSpawned() && player.getPosition().intersects(ammoPickup.getPosition()))
			{
				bulletsSpare+=(ammoPickup.gotIt());
				reload.play();
			}


			// size up the health bar
			healthBar.setSize(Vector2f(player.getHealth() * 6, 40));

			// Increment the number of frames since the last HUD calculation
			framesSinceLastHUDUpdate++;
			// Calculate FPS every fpsMeasurementFrameInterval frames
			if (framesSinceLastHUDUpdate > fpsMeasurementFrameInterval)
			{

				// Update game HUD text
				std::stringstream ssAmmo;
				std::stringstream ssScore;
				std::stringstream ssHiScore;
				std::stringstream ssWave;
				std::stringstream ssZombiesAlive;

				// Update the ammo text
				ssAmmo << bulletsInClip << "/" << bulletsSpare;
				ammoText.setString(ssAmmo.str());

				// Update the score text
				ssScore << "Score:" << score;
				scoreText.setString(ssScore.str());

				// Update the high score text
				ssHiScore << "Hi Score:" << hiScore;
				hiScoreText.setString(ssHiScore.str());

				// Update the wave
				ssWave << "Wave:" << wave;
				waveNumberText.setString(ssWave.str());

				// Update the high score text
				ssZombiesAlive << "Zombies:" << numZombiesAlive;
				zombiesRemainingText.setString(ssZombiesAlive.str());

				framesSinceLastHUDUpdate = 0;
			}// End HUD update


		}// End Update

		 //Draw
		if (state == State::PLAYING) {
			window.clear();
			window.setView(mainView);
			window.draw(background, &textureBackground);
			for (int i = 0; i < numZombies; i++) {
				window.draw(zombies[i].getSprite());
			}
			for (int i = 0; i < 100; i++) {
				if (bullets[i].isInFlight()) {
					window.draw(bullets[i].getShape());
				}
			}
			window.draw(player.getSprite());
			if (ammoPickup.isSpawned()) {
				window.draw(ammoPickup.getSprite());
			}
			if (healthPickup.isSpawned()) {
				window.draw(healthPickup.getSprite());
			}
			window.draw(spriteCrosshair);

			// Switch to the HUD view
			window.setView(hudView);

			// Draw all the HUD elements
			window.draw(spriteAmmoIcon);
			window.draw(ammoText);
			window.draw(scoreText);
			window.draw(hiScoreText);
			window.draw(healthBar);
			window.draw(waveNumberText);
			window.draw(zombiesRemainingText);
		}

		if (state == State::LEVELING_UP) {
			window.draw(spriteGameOver);
			window.draw(levelUpText);
		}

		if (state == State::PAUSED) {
			window.draw(pausedText);
		}

		if (state == State::GAME_OVER) {
			window.draw(spriteGameOver);
			window.draw(gameOverText);
			window.draw(scoreText);
			window.draw(hiScoreText);
		}
		window.display();

	}

	delete[] zombies;

	return 0;
}