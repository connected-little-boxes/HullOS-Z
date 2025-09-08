#include "Leds.h"
#include "messages.h"

bool close_to(float a, float b);

Leds::Leds(int inWidth, int inHeight, 
			void (*inShow)(),
			void (*inSetPixel)(int no, float r, float g, float b))
{
	ledWidth = inWidth;
	ledHeight = inHeight;

	normWidth = 1.0; // width of display is always 1
	normHeight = (float)ledHeight / (float)ledWidth;

	show = inShow;
	setPixel = inSetPixel;

	leds = new Led *[ledWidth];

	for (int i = 0; i < ledWidth; i++)
	{
		leds[i] = new Led[ledHeight];
	}
}

void Leds::display(float brightness)
{
	int ledNo = 0;
	for (int y = 0; y < ledHeight; y++)
	{
		for (int x = 0; x < ledWidth; x++)
		{
			float r = (leds[x][y].colour.Red * brightness);
			float g = (leds[x][y].colour.Green * brightness);
			float b = (leds[x][y].colour.Blue * brightness);
			setPixel(ledNo, r, g, b);
			ledNo++;
		}
	}
	show();
}

void Leds::clear(Colour colour)
{
	for (int y = 0; y < ledHeight; y++)
	{
		for (int x = 0; x < ledWidth; x++)
		{
			leds[x][y].colour = colour;
		}
	}
}

void Leds::wash(Colour colour)
{
	for (int y = 0; y < ledHeight; y++)
	{
		for (int x = 0; x < ledWidth; x++)
		{
			Colour led = leds[x][y].colour;
			if(led.Red==0 && led.Blue==0 && led.Green==0){
				leds[x][y].colour = colour;
			}
		}
	}
}


void Leds::dump()
{
	displayMessage("Leds width:%d height:%d\n  ", ledWidth, ledHeight);
	for (int y = 0; y < ledHeight; y++)
	{
		for (int x = 0; x < ledWidth; x++)
		{
			displayMessage("     r:%f g:%f b:%f\n",
						  leds[x][y].colour.Red,
						  leds[x][y].colour.Green,
						  leds[x][y].colour.Blue);
		}
	}
	displayMessage("\n");
	}



void Leds::renderLight(float sourceX, float sourceY, Colour colour, float brightness, float opacity)
{

	int intX = int(sourceX);
	int intY = int(sourceY);

	for (int xOffset = -1; xOffset <= 1; xOffset++)
	{
		int px = intX + xOffset;
		int writeX;

		if (px >= ledWidth)
		{
			writeX = px - ledWidth;
		}
		else
		{
			if (px < 0)
			{
				writeX = px + ledWidth;
			}
			else 
			{
				writeX=px;
			}
		}

		for (int yOffset = -1; yOffset <= 1; yOffset++)
		{
			int py = intY + yOffset;
			int writeY;

			if (py >= ledHeight)
			{
				writeY = py - ledHeight;
			}
			else
			{
				if (py < 0)
				{
					writeY = py + ledHeight;
				}
				else
				{
					writeY = py;
				}
			}

			// each pixel is at the centre of a 1*1 square
			// find the distance from this pixel to the light
			float dx = sourceX - (px + 0.5);
			float dy = sourceY - (py + 0.5);
			//float dist = sqrt((dx * dx) + (dy * dy));
			float dist = (dx * dx) + (dy * dy);

			// The closer the pixel is to the light - the brighter it is
			// pixels more than 1 from the light are not lit
			if (dist < 1)
			{
				// create a factor from the distance
				float factor = 1 - dist;

				// apply the factor to the colour value
				float rs = colour.Red * brightness * factor;
				float gs = colour.Green * brightness * factor;
				float bs = colour.Blue * brightness * factor;

				leds[writeX][writeY].AddColourValues(rs, gs, bs, opacity);
			}
		}
	}
	return;
}
#ifdef OLD_CODE

static uint16_t W3[256][3]; // Q0.16 weights for phases 0..255, taps -1,0,+1
static bool w3Built = false;

static void buildW3() {
    if (w3Built) return;
    for (int t = 0; t < 256; ++t) {
        float u = t / 255.0f;
        auto k = [](float x){ x=fabsf(x); return (x<=1.f) ? 0.5f*(1.f+cosf((float)M_PI*x)) : 0.f; };
        float w0 = k(1.0f - u); // i-1
        float w1 = k(u);        // i
        float w2 = k(1.0f + u); // i+1
        float sum = w0 + w1 + w2; if (sum <= 0) { w1 = 1; w0 = w2 = 0; sum = 1; }
        uint32_t q0 = (uint32_t)lrintf((w0/sum)*65536.f);
        uint32_t q1 = (uint32_t)lrintf((w1/sum)*65536.f);
        uint32_t q2 = (uint32_t)lrintf((w2/sum)*65536.f);
        // fix rounding to sum exactly to 65536
        int32_t fix = 65536 - (int32_t)(q0 + q1 + q2);
        q1 += fix;
        W3[t][0] = (uint16_t)q0; W3[t][1] = (uint16_t)q1; W3[t][2] = (uint16_t)q2;
    }
    w3Built = true;
}


void Leds::renderLight(float sourceX, float sourceY, Colour colour, float brightness, float opacity)
{
    buildW3();

    // Clamp to panel (wrap on write below)
    if (ledWidth  > 1) { if (sourceX < 0) sourceX = 0; else if (sourceX > ledWidth  - 1e-6f) sourceX = ledWidth  - 1e-6f; }
    if (ledHeight > 1) { if (sourceY < 0) sourceY = 0; else if (sourceY > ledHeight - 1e-6f) sourceY = ledHeight - 1e-6f; }

    int ix = (int)floorf(sourceX);
    int iy = (int)floorf(sourceY);
    int px = (int)lrintf((sourceX - ix) * 255.0f);  // 0..255
    int py = (int)lrintf((sourceY - iy) * 255.0f);  // 0..255
    const uint16_t* Wx = W3[px]; // Q0.16 weights [-1,0,+1]
    const uint16_t* Wy = W3[py];

    // Pre-multiply once
    float r = colour.Red   * brightness;
    float g = colour.Green * brightness;
    float b = colour.Blue  * brightness;

    for (int dy = -1; dy <= +1; ++dy) {
        int y = iy + dy;
        if (y < 0) y += ledHeight; else if (y >= ledHeight) y -= ledHeight;
        uint32_t wy = Wy[dy+1]; // Q0.16
        for (int dx = -1; dx <= +1; ++dx) {
            int x = ix + dx;
            if (x < 0) x += ledWidth; else if (x >= ledWidth) x -= ledWidth;
            uint32_t wx = Wx[dx+1]; // Q0.16
            // combined weight in Q0.16
            uint32_t w = (uint32_t)(((uint64_t)wx * (uint64_t)wy) >> 16); // Q0.16

            // Convert to float just once per write (still cheap on RP2040)
            float wf = (float)w / 65536.0f;

            leds[x][y].AddColourValues(r * wf, g * wf, b * wf, opacity);
        }
    }
}
#endif
