#include <SFML/Graphics.hpp>

int main()
{
	sf::RenderWindow window(sf::VideoMode({200, 200}), "SFML works!"); //creiamo finestra con risoluzione 200x200
	sf::CircleShape shape(100); //cpu genera poligono di lati sufficienti a sembrare tondo di raggio 100px, SFML lo salva in RAM
	shape.setFillColor(sf::Color::Green); //cambiamo proprietà del cerchio

	while(window.isOpen()) //main loop fichè finestra è aperta
	{
		while(const std::optional event= window.pollEvent()) //prende evento dalla coda degli eventi
		{
			if(event->is<sf::Event::Closed>()) //se l'evento è il tasto X per ciudere finestra
				window.close(); //la finestra viene chiusa
		}

		window.clear(); //cancellato il back buffer
		window.draw(shape); //dati passati da RAM a VRAM della GPU che prende i vertici e fa rasterizzazione scrivendoli nel back buffer
		window.display(); //scambia il back buffer con il front buffer mostrando a schermo il cerchio
	}
}
