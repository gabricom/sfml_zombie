#pragma once
#ifndef TEXTURE_HOLDER_H
#define TEXTURE_HOLDER_H

#include <SFML/Graphics.hpp>
#include <map>

class TextureHolder {
private:
	std::map<std::string, sf::Texture> m_Textures;
	static TextureHolder* m_s_Instance;
public:
	TextureHolder();
	static sf::Texture &getTexture(std::string const& filename);
};


#endif