#include "stdafx.h"
#include <SFML/Graphics.hpp>
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

	sf::RenderWindow window( sf::VideoMode(resolution.x,resolution.y),
		"Zombie Arena",sf::Style::Default );

	sf::View mainView( sf::FloatRect(0,0,resolution.x/2,resolution.y/2) ) ;

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
	int bulletsInClip = 6;
	int clipSize = 6;
	float fireRate = 3;

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
				}

				if (state == State::PLAYING) {
					if (event.key.code == Keyboard::R) {
						if (bulletsSpare >= clipSize) {
							bulletsInClip = clipSize;
							bulletsSpare -= clipSize;
						}
						else if(bulletsSpare > 0){
							bulletsInClip = bulletsSpare;
							bulletsSpare = 0;
						}
						else {

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
				}
			}

		}//End WASD Handling

		 //Handle Levelling up
		if (state == State::LEVELING_UP) {
			if (event.key.code == sf::Keyboard::Num1) {
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num2) {
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num3) {
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num4) {
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num5) {
				state = State::PLAYING;
			}
			if (event.key.code == sf::Keyboard::Num6) {
				state = State::PLAYING;
			}

			if (state == State::PLAYING) {

				arena.width = 1000;
				arena.height = 1000;
				arena.left = 0;
				arena.top = 0;
				int tileSize = createBackground(background,arena);
				player.spawn(arena, resolution, tileSize);
				healthPickup.setArena(arena);
				ammoPickup.setArena(arena);

				numZombies = 10;
				delete[] zombies;
				zombies = createHorde(numZombies, arena);
				numZombiesAlive = numZombies;

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
							}
						}
					}
				}
			}

			//Gestione collisione zombie/giocatore
			for (int i = 0; i < numZombies; i++) {
				if (zombies[i].isAlive() && player.getPosition().intersects(zombies[i].getPosition())) {
					if (player.hit(gameTimeTotal)) {

					}
					if (player.getHealth() <= 0) {
						state = State::GAME_OVER;
					}
				}
			}

			//Gestione collisione pickup/giocatore
			if (healthPickup.isSpawned() && player.getPosition().intersects(healthPickup.getPosition()))
			{
				player.increaseHealthLevel(healthPickup.gotIt());
			}
			if (ammoPickup.isSpawned() && player.getPosition().intersects(ammoPickup.getPosition()))
			{
				bulletsSpare+=(ammoPickup.gotIt());
			}

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
		}

		if (state == State::LEVELING_UP) {

		}

		if (state == State::PAUSED) {

		}
		window.display();

	}

	delete[] zombies;

	return 0;
}