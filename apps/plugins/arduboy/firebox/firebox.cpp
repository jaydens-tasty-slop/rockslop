// Firebox by Jonathan Howard (MIT). YP3 port: paint8Pixels goes to a frame buffer.
#include <Arduino.h>
#include <Arduboy2.h>
#include "images.h"

Arduboy2Core arduboy;
static uint8_t fb[1024];
static uint16_t fbpos;
static void paint8(uint8_t b) { fb[fbpos++ & 1023] = b; }


constexpr int displayWidth = 64;
constexpr int displayHeight = 32;
constexpr int particleShift = 2;
constexpr int particleMaxX = displayWidth << particleShift;
constexpr int particleMaxY = displayHeight << particleShift;
constexpr uint8_t maxFireValue = 4;
constexpr int numDitherValues = 5;
constexpr int maxParticles = 16;
constexpr int gravity = 1;
constexpr int maxGravity = 8;
constexpr int numParticleTypes = 10;
constexpr int firstPreviewParticle = 2;

uint8_t pixels[displayWidth * displayHeight];
int16_t previewX = displayWidth / 2;

struct ParticleType
{
	const unsigned char* imageData;
	uint8_t startingFuel;
	uint8_t startingResistance;
	uint8_t explosionSize;
	uint8_t explosionType;
	
	const unsigned char* getImageData()
	{
		return pgm_read_ptr(&imageData);
	}
	uint8_t fuel()
	{
		return pgm_read_byte(&startingFuel);
	}
	uint8_t resistance()
	{
		return pgm_read_byte(&startingResistance);
	}
	
	uint8_t getExplosionSize()
	{
		return pgm_read_byte(&explosionSize);
	}
	uint8_t getExplosionType()
	{
		return pgm_read_byte(&explosionType);
	}
};

const ParticleType particleTypes[numParticleTypes] PROGMEM =
{
	{
		fireballImageData,
		25,		// Fuel
		0,		// Resistance
	},
	{
		fireballImageData,
		25,		// Fuel
		0,		// Resistance
		3,		// Explosion size
		0,		// Explosion type
	},
	{
		logImageData,
		255,	// Fuel
		4,		// Resistance
	},
	{
		paperImageData,
		50,		// Fuel
		1,		// Resistance
	},
	{
		arduboyImageData,
		50,		// Fuel
		40,		// Resistance
		4,		// Explosion size
		0,		// Explosion type
	},
	{
		faceImageData,
		50,		// Fuel
		1,		// Resistance
	},
	{
		batteryImageData,
		5,		// Fuel
		40,		// Resistance
		3,		// Explosion size
		1,		// Explosion type
	},
	{
		mouseImageData,
		20,		// Fuel
		10,		// Resistance
		3,		// Explosion size
		0,		// Explosion type
	},
	{
		glassesImageData,
		10,		// Fuel
		20,		// Resistance
	},
	{
		bombImageData,
		50,		// Fuel
		4,		// Resistance
		5,		// Explosion size
		1,		// Explosion type
	},
	
};

struct Particle
{
	uint8_t type;
	int16_t x, y;
	int16_t oldX, oldY;
	uint8_t fuel;
	uint8_t resistance;
	
	const ParticleType& particleType() { return particleTypes[type]; }
	
	uint8_t width() { return drawWidth() << particleShift; }
	uint8_t height() { return drawHeight() << particleShift; }

	uint8_t drawWidth() 
	{ 
		return pgm_read_byte(&particleType().getImageData()[0]);
	}
	uint8_t drawHeight() 
	{ 
		return pgm_read_byte(&particleType().getImageData()[1]);
	}

};

Particle particles[maxParticles];
Particle& previewParticle = particles[0];

/*
	--
	--
	
	-X
	--
	
	-X
	X-
	
	XX
	X-
	
	XX
	XX
*/


const uint8_t ditherPatterns[] PROGMEM =
{
	0b00000000,
	0b00000000,
	0b00000010,
	0b00000011,
	0b00000011,

	0b00000000,
	0b00000000,
	0b00001000,
	0b00001100,
	0b00001100,


	0b00000000,
	0b00000000,
	0b00100000,
	0b00110000,
	0b00110000,
           

	0b00000000,
	0b00000000,
	0b10000000,
	0b11000000,
	0b11000000,
           
	0b00000000,
	0b00000001,
	0b00000001,
	0b00000001,
	0b00000011,

	0b00000000,
	0b00000100,
	0b00000100,
	0b00000100,
	0b00001100,

	0b00000000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00110000,

	0b00000000,
	0b01000000,
	0b01000000,
	0b01000000,
	0b11000000,
	
};

uint8_t buttonsState = 0;
uint8_t lastButtonsState = 0;

inline bool justPressed(uint8_t buttonMask)
{
	return ((buttonsState & buttonMask) == buttonMask
	&& (lastButtonsState & buttonMask) == 0);
}

inline bool isPressed(uint8_t buttonMask)
{
	return (buttonsState & buttonMask) == buttonMask;
}

void setup();
void loop();
void setup() 
{
	arduboy.boot();
	arduboy.safeMode();
	
	for(Particle& p : particles)
	{
		p.fuel = 0;
	}
	
	previewParticle.type = firstPreviewParticle;
}

inline uint16_t generateRandomNumber()
{
	static uint16_t xs = 1;

	xs ^= xs << 7;
	xs ^= xs >> 9;
	xs ^= xs << 8;
	return xs;
}

Particle* spawnParticle(uint8_t type, int16_t x, int16_t y)
{
	for(Particle& p : particles)
	{
		if(p.fuel == 0)
		{
			const ParticleType& particleType = particleTypes[type];
			p.type = type;
			p.oldX = p.x = x;
			p.oldY = p.y = y;
			p.fuel = particleType.fuel();
			p.resistance = particleType.resistance();
			return &p;
		}
	}
	return nullptr;
}

void explode(uint8_t type, uint8_t count, int16_t x, int16_t y)
{
	for(uint8_t n = 0; n < count; n++)
	{
		Particle* p = spawnParticle(type, x, y);
		if(p)
		{
			p->oldX += (generateRandomNumber() & 31) - 16;
			p->oldY += (generateRandomNumber() & 15);
		}
		else return;
	}
}

void spreadFire(int src)
{
	uint8_t rand = (uint8_t)(generateRandomNumber() & 3);
	
	int dst = src - rand + 1 - displayWidth;
	
	if(dst < 0 || dst >= displayWidth * displayHeight)
		return;
	
	if(pixels[src] > 0)
		pixels[dst] = pixels[src] - (rand & 1);
	else
		pixels[dst] = 0;
}

inline void paintDisplay()
{
	uint32_t total = 0;
	fbpos = 0;
	
	for(int y = 0; y < displayHeight; y += 4)
	{
		int index = (y * displayWidth);
		for(int x = 0; x < displayWidth; x++)
		{
			int ditherBase = 0;
			uint8_t output = 0;
			
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index]]);
			ditherBase += numDitherValues;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index + displayWidth]]);
			ditherBase += numDitherValues;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index + 2 * displayWidth]]);
			ditherBase += numDitherValues;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index + 3 * displayWidth]]);
			ditherBase += numDitherValues;
			
			paint8(output);
			
			output = 0;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index]]);
			ditherBase += numDitherValues;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index + displayWidth]]);
			ditherBase += numDitherValues;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index + 2 * displayWidth]]);
			ditherBase += numDitherValues;
			output |= pgm_read_byte(&ditherPatterns[ditherBase + pixels[index + 3 * displayWidth]]);
			ditherBase += numDitherValues;
			
			paint8(output);
			index++;
			
			total += output;
		}
	}
	
	arduboy.paintScreen(fb);
	static bool flicker = false;
	uint8_t overallFireStrength = (uint8_t) (total / 512);
	flicker = !flicker;
	if(flicker)
		arduboy.setRGBled(overallFireStrength / 2, 0, 0);
	else
		arduboy.setRGBled(overallFireStrength, 0, 0);
}

inline void clearParticles()
{
	for(Particle& p : particles)
	{
		if(p.fuel > 0)
		{
			int16_t drawX = p.x >> particleShift;
			int16_t drawY = p.y >> particleShift;

			const uint8_t* imageDataPtr = p.particleType().getImageData() + 2;
			
			for(int y = 0; y < p.drawHeight(); y++)
			{
				for(int x = 0; x < p.drawWidth(); x++)
				{
					if(pgm_read_byte(imageDataPtr) != 0xff)
					{
						int16_t outX = (drawX + x);
						int16_t outY = (drawY + y);
						if(outX >= 0 && outX < displayWidth && outY >= 0 && outY < displayHeight)
						{
							pixels[outY * displayWidth + outX] = 0;
						}
					}
					imageDataPtr++;
				}
			}
		}
	}
}

inline void moveParticles()
{
	static uint8_t counter = 0;
	counter++;
	
	for(Particle& p : particles)
	{
		if(p.fuel > 0)
		{
			int16_t velX = p.x - p.oldX;
			int16_t velY = p.y - p.oldY;
			p.oldX = p.x;
			p.oldY = p.y;
			
			if(velY < maxGravity)
			{
				velY += gravity;
			}
			
			if((counter & 7) == 0)
			{
				if(velX < 0)
					velX ++;
				if(velX > 0)
					velX--;
			}
			
			p.x += velX;
			p.y += velY;
		}
	}	
}

inline void constrainParticles()
{
	constexpr int maxIterations = 10;
	bool collided = false;
	
	for(int it = 0; it < maxIterations && (it == 0 || collided); it++)
	{
		collided = false;
		
		for(int x = 0; x < maxParticles; x++)
		{
			Particle& p = particles[x];
			
			if(p.fuel > 0)
			{
				for(int y = 0; y < maxParticles; y++)
				{
					Particle& other = particles[y];
					if(x != y && other.fuel > 0)
					{
						if(other.x > p.x + p.width())
							continue;
						if(other.y > p.y + p.height())
							continue;
						if(p.x > other.x + other.width())
							continue;
						if(p.y > other.y + other.height())
							continue;
						
						collided = true;
						
						// Colliding 
						int16_t correctionLeft = (other.x - p.width()) - p.x;
						int16_t correctionRight = (other.x + other.width()) - p.x;
						int16_t correctionUp = (other.y - p.height()) - p.y;
						int16_t correctionDown = (other.y + other.height()) - p.y;
						
						int16_t correctionX, correctionY, absCorrectionX, absCorrectionY;
						if(-correctionLeft < correctionRight)
						{
							correctionX = correctionLeft;
							absCorrectionX = -correctionLeft;
						}
						else
						{
							correctionX = correctionRight;
							absCorrectionX = correctionRight;
						}
						if(-correctionUp < correctionDown)
						{
							correctionY = correctionUp;
							absCorrectionY = -correctionUp;
						}
						else
						{
							correctionY = correctionDown;
							absCorrectionY = correctionDown;
						}
						
						if(absCorrectionX < absCorrectionY)
						{
							other.x -= (correctionX / 2);
							correctionX -= (correctionX / 2);
							p.x += correctionX;
						}
						else
						{
							other.y -= (correctionY / 2);
							correctionY -= (correctionY / 2);
							p.y += correctionY;
						}
					}
				}
				
				if(p.x < 0)
				{
					p.x = 0;
				}
				if(p.x + p.width() >= particleMaxX)
				{
					p.x = particleMaxX - p.width();
				}
				if(p.y + p.height() >= particleMaxY)
				{
					p.y = particleMaxY - p.height();
				}
			}
		}
	}
}


inline void drawLitParticles()
{
	for(Particle& p : particles)
	{
		if(p.fuel > 0 && p.resistance == 0)
		{
			int16_t drawX = p.x >> particleShift;
			int16_t drawY = p.y >> particleShift;
			
			const uint8_t* imageDataPtr = p.particleType().getImageData() + 2;

			for(int y = 0; y < p.drawHeight(); y++)
			{
				for(int x = 0; x < p.drawWidth(); x++)
				{
					if(pgm_read_byte(imageDataPtr) != 0xff)
					{
						int16_t outX = (drawX + x);
						int16_t outY = (drawY + y);
						if(outX >= 0 && outX < displayWidth && outY >= 0 && outY < displayHeight)
						{
							pixels[outY * displayWidth + outX] = maxFireValue;
						}
					}
					
					imageDataPtr++;
				}
			}
		}
	}
}

inline void animateFire()
{
	if(isPressed(A_BUTTON))
	{
		for(int x = 0; x < displayWidth; x++)
		{
			pixels[(displayHeight - 1) * displayWidth + x] = maxFireValue;
		}
	}
	else
	{
		for(int x = 0; x < displayWidth; x++)
		{
			int dist = (displayWidth / 2) - x;
			if(dist < 0) 
				dist = -dist;
			int strength = maxFireValue - dist;
			if(strength < 0)
				strength = 0;
			pixels[(displayHeight - 1) * displayWidth + x] = (uint8_t) strength;
		}
	}

	for(int x = 0; x < displayWidth; x++)
	{
		for(int y = 1; y < displayHeight; y++)
		{
			spreadFire(y * displayWidth + x);
		}
	}
}

inline void igniteParticles()
{
	for(Particle& p : particles)
	{
		if(p.fuel > 0 && p.resistance > 0)
		{
			bool ignite = false;
			int16_t drawX = p.x >> particleShift;
			int16_t drawY = p.y >> particleShift;

			for(int y = -1; y <= p.drawHeight() && !ignite; y++)
			{
				int16_t outY = (drawY + y);
				
				if(outY >= 0 && outY < displayHeight)
				{
					int16_t outX = drawX - 1;
					
					if(outX >= 0 && outX < displayWidth && pixels[outY * displayWidth + outX])
					{
						ignite = true;
						break;
					}
					
					outX = drawX + p.drawWidth();

					if(outX >= 0 && outX < displayWidth && pixels[outY * displayWidth + outX])
					{
						ignite = true;
						break;
					}
				}
			}
			
			for(int x = -1; x <= p.drawWidth() && !ignite; x++)
			{
				int16_t outX = (drawX + x);
				
				if(outX >= 0 && outX < displayWidth)
				{
					int16_t outY = drawY - 1;
					
					if(outY >= 0 && outY < displayHeight && pixels[outY * displayWidth + outX])
					{
						ignite = true;
						break;
					}
					
					outY = drawY + p.drawHeight();

					if(outY >= 0 && outY < displayHeight && pixels[outY * displayWidth + outX])
					{
						ignite = true;
						break;
					}
				}
			}
			
			if(ignite)
			{
				p.resistance--;
			}
		}
	}
}

inline void drawParticles()
{
	for(Particle& p : particles)
	{
		if(p.fuel > 0)
		{
			int16_t drawX = p.x >> particleShift;
			int16_t drawY = p.y >> particleShift;
			
			const uint8_t* imageDataPtr = p.particleType().getImageData() + 2;

			for(int y = 0; y < p.drawHeight(); y++)
			{
				for(int x = 0; x < p.drawWidth(); x++)
				{
					uint8_t imagePixel = pgm_read_byte(imageDataPtr);
					
					if(imagePixel != 0xff)
					{
						int16_t outX = (drawX + x);
						int16_t outY = (drawY + y);
						if(outX >= 0 && outX < displayWidth && outY >= 0 && outY < displayHeight)
						{
							if(imagePixel)
							{
								pixels[outY * displayWidth + outX] = maxFireValue;
							}
							else
							{
								pixels[outY * displayWidth + outX] = 0;
							}
						}
					}
					
					imageDataPtr++;
				}
			}
			
			if(p.resistance == 0)
			{
				p.fuel--;
				if(p.fuel == 0)
				{
					if(p.type)
					{
						explode(p.particleType().getExplosionType(), p.particleType().getExplosionSize(), p.x + p.width() / 2, p.y + p.height() / 2);
					}
				}
			}
		}
	}
}

inline void updateInput()
{
	buttonsState = arduboy.buttonsState();
	
	if(justPressed(B_BUTTON))
	{
		previewParticle.oldY = previewParticle.y = -previewParticle.height() - 1;
		spawnParticle(previewParticle.type, previewX, 0);
	}
	
	/*if(justPressed(A_BUTTON))
	{
		explode(1, 3, particleMaxX / 2, particleMaxY / 2);
	}*/
	
	/*if(justPressed(A_BUTTON))
	{
		for(int n = 1; n < maxParticles; n++)
		{
			Particle& p = particles[n];
			if(p.fuel > 0)
			{
				p.fuel = 2;
				p.resistance = 0;
			}
		}
	}*/
	
	if(justPressed(UP_BUTTON))
	{
		previewParticle.type--; 
		if(previewParticle.type < firstPreviewParticle)
		{
			previewParticle.type = numParticleTypes - 1;;
		}
	}
	if(justPressed(DOWN_BUTTON))
	{
		previewParticle.type++; 
		if(previewParticle.type >= numParticleTypes)
		{
			previewParticle.type = firstPreviewParticle;
		}
	}

	if(isPressed(LEFT_BUTTON))
	{
		previewX -= (1 << particleShift);
		if(previewX < 0)
			previewX = 0;
	}
	if(isPressed(RIGHT_BUTTON))
	{
		previewX += (1 << particleShift);
		if(previewX + previewParticle.width() >= particleMaxX)
		{
			previewX = particleMaxX - previewParticle.width();
		}
	}

	previewParticle.x = previewParticle.oldX = previewX;
	if(previewParticle.y > 0)
	{
		previewParticle.oldY = previewParticle.y = 0;
	}
	previewParticle.fuel = previewParticle.particleType().fuel();
	previewParticle.resistance = previewParticle.particleType().resistance();
	
	lastButtonsState = buttonsState;
}

void loop() 
{
	constexpr int updateRate = 1000 / 30;
	static unsigned long lastFrameTime = 0;
	
	if((millis() - lastFrameTime) < updateRate)
	{
		return;
	}
	lastFrameTime = millis();

	
	// 1. clear particles
	clearParticles();
	
	// 2. move particles
	moveParticles();
	
	// 3. constrain particles
	constrainParticles();
	
	// 4. update input and preview particle
	updateInput();
	
	// 6. draw lit particles
	drawLitParticles();
	
	// 7. spread fire
	animateFire();
	
	// 8. ignite any particles
	igniteParticles();
	
	// 9. draw particles
	drawParticles();
	
	// 10. draw to display
	paintDisplay();
}
