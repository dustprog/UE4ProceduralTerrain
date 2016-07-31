

#include "Fero.h"
#include "Heightmap.h"

#include <random>
#include <fstream>
using namespace std;

AHeightmap::AHeightmap(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

Heightmap::Heightmap(int32 width, int32 height)
:  m_Width(width), m_Height(height)
{
	m_natural_profile = new Heightmap_1D(3, 0);
	m_natural_profile->SetValue(1, 91 * 15);
	m_natural_profile->SetValue(2, 255 * 15);

	m_std_noise = new Amplitudes(4096);
	m_std_noise->AddAmplitude(1, 3 * 15);
	m_std_noise->AddAmplitude(2, 7 * 15);
	m_std_noise->AddAmplitude(4, 10 * 15);
	m_std_noise->AddAmplitude(8, 20 * 15);
	m_std_noise->AddAmplitude(16, 50 * 15);
	m_std_noise->AddAmplitude(32, 75 * 15);
	m_std_noise->AddAmplitude(64, 150 * 15);
	m_std_noise->AddAmplitude(128, 250 * 15);
	m_std_noise->AddAmplitude(256, 400 * 15);
	m_std_noise->AddAmplitude(512, 600 * 15);
	m_std_noise->AddAmplitude(1024, 1000 * 15);
	m_std_noise->AddAmplitude(2048, 1400 * 15);
	m_std_noise->AddAmplitude(4096, 2000 * 15);
}

void Heightmap::Load(FString heightmapName, int32 width, int32 height)
{
	m_Width = width;
	m_Height = height;
}

void Heightmap::LoadDoublePass()
{
	int32 width = m_Width;
	int32 height = m_Height;
	int32 smoothness = 1;

	bool flipped = false;

	if (width < height)
	{
		flipped = true;

		int32 temp = width;
		width = height;
		height = width;
	}

	base = new Heightmap_2D(width, height, 0);

	// create one peak into the middle of the map
	Heightmap_2D* pass = new Heightmap_2D(height / 3, height, 0);

	pass->RadialGradient(1, height / 2, width > height ? height / 3 : width / 3, 1200, 0, true);

	pass->Gradient(1, 0, 1, height / 6, 800, 0, false);
	pass->Gradient(0, 5 * height / 6, 0, height - 1, 0, 800, false);

	Heightmap_2D* copy = pass->Clone();

	copy->Flip(PT_VERTICAL);

	pass->Union(copy);

	copy = NULL;

	Heightmap_1D* profile = pass->GetProfile(PT_VERTICAL, 0);

	base->Project(profile, PT_HORIZONTAL);

	profile = NULL;

	base->IntersectionTo(pass, width / 3 - pass->Width() / 2, 0);
	base->IntersectionTo(pass, 2 * width / 3 - pass->Width() / 2, 0);

	pass = NULL;

	base->Smooth((width > height ? height : width) / 10);

	Heightmap_1D* profile_shift = new Heightmap_1D(width, 0);

	profile_shift->Noise(width / 10, width / 2, m_std_noise);
	profile_shift->ScaleValuesTo(-height / 24, height / 24);

	base->Shift(profile_shift, PT_VERTICAL, PT_DISCARD_AND_FILL);

	base->Smooth(20);

	profile_shift = NULL;

	Heightmap_2D* mask = base->Clone();

	mask->Clamp(200, PT_MAX_HEIGHT);
	mask->ScaleValuesTo(0, PT_MAX_HEIGHT);
	mask->Clamp(PT_MAX_HEIGHT / 5, PT_MAX_HEIGHT);

	Heightmap_2D* noise = new Heightmap_2D(width, height, 0);

	noise->Noise(smoothness, width > height ? height / 8 : width / 8, m_std_noise);

	noise->ScaleValuesTo(-300, 300);

	base->AddMapMasked(noise, mask, false);

	base->Add(-base->Min() + 1);

	if (flipped) base->Rotate(270, false);
}

void Heightmap::LoadValley()
{
	int32 width = m_Width;
	int32 height = m_Height;

	int32 valley_width = 2;
	int32 smoothness = 1;
	int32 feature_size = 1;

	base = new Heightmap_2D(width, height, 0);

	// Create a shore->sea profile
	Heightmap_1D* profile_height = new Heightmap_1D(10, 0);

	profile_height->SetValue(0, -800);
	profile_height->SetValue(1, -200);
	profile_height->SetValue(2, 100);
	profile_height->SetValue(3, 500);
	profile_height->SetValue(4, 1000);
	profile_height->SetValue(5, 1500);
	profile_height->SetValue(6, 1500);
	profile_height->SetValue(7, 1500);
	profile_height->SetValue(8, 1500);
	profile_height->SetValue(9, 1500);

	// Assemble the valley profile from several parts
	int valley_profile_width = 100;

	Heightmap_1D* profile_valley = new Heightmap_1D(valley_profile_width, 1500);

	valley_width = 3 + valley_width;

	profile_valley->SetValueInRange(valley_profile_width / 2 - valley_width, valley_profile_width / 2 + valley_width, 50);

	profile_valley->Gradient(valley_profile_width / 2 - valley_profile_width / 6, valley_profile_width / 2 - valley_width - 1, 300, 1500, false);
	profile_valley->Gradient(valley_profile_width / 2 + valley_width + 1, valley_profile_width / 2 + valley_profile_width / 6, 1500, 300, false);

	base->Project(profile_height, PT_VERTICAL);

	Heightmap_2D* valley = new Heightmap_2D(width, height, 0);

	valley->Project(profile_valley, PT_HORIZONTAL);

	// Combine the two profiles
	base->Intersection(valley);

	// Create some meandres on the river using the shift with random noise
	Heightmap_1D* profile_shift = new Heightmap_1D(800, 0);

	profile_shift->Noise(64 < width / 10 ? 64 : width / 10, height / 8, m_std_noise);

	profile_shift->ScaleValuesTo(-width / 11, width / 11);

	base->Shift(profile_shift, PT_VERTICAL, PT_DISCARD_AND_FILL);

	// Keep the original valley for later use as mask
	valley->Shift(profile_shift, PT_VERTICAL, PT_DISCARD_AND_FILL);

	// Meld the shapes a little bit
	base->Smooth(width > height ? height / 50 : width / 50);

	valley->Smooth(width > height ? height / 50 : width / 50);

	// Noise overlay
	Heightmap_2D* noise = new Heightmap_2D(width, height, 0);

	noise->Noise(smoothness, width > height ? height / (18 - 4 * feature_size) : width / (18 - 4 * feature_size), m_std_noise);

	noise->ScaleValuesTo(-1400, 1400);

	valley->ScaleValuesTo(2 * PT_MAX_HEIGHT / 25, PT_MAX_HEIGHT);

	base->AddMapMasked(noise, valley, false);

	Path* path = profile_shift->ToPath(width);

	path->Move(0, height / 2 - (valley_width - 2) * height / 100);

	signed short max = base->GetMaxValueOnPath(path);

	base->Add(-max);

	base->TransformValues(m_natural_profile, true);
}

void Heightmap::LoadMountains()
{
	// load map parameters
	int32 width = m_Width;
	int32 height = m_Height;
	int32 smoothness = 1 << 1;
	int32 feature_size = 1;

	// create a radial gradient with height 1 in the center and height 1200 on the outer rim
	base = new Heightmap_2D(width, height, 0);

	Heightmap_1D* profile_height = new Heightmap_1D(10, 0);

	profile_height->SetValue(0, -800);
	profile_height->SetValue(1, -200);
	profile_height->SetValue(2, 100);
	profile_height->SetValue(3, 500);
	profile_height->SetValue(4, 1000);
	profile_height->SetValue(5, 1500);
	profile_height->SetValue(6, 1500);
	profile_height->SetValue(7, 1500);
	profile_height->SetValue(8, 1500);
	profile_height->SetValue(9, 1500);

	//base->Project(profile_height, PT_VERTICAL);

	// add smaller islands on top of the base shape
	Heightmap_2D* mountains = new Heightmap_2D(width, height, 0);

	mountains->Noise(smoothness, width > height ? height / (18 - 4 * feature_size) : width / (18 - 4 * feature_size), m_std_noise);

	mountains->ScaleValuesTo(-1400, 1400);

	// combine the maps
	base->AddMap(mountains);

	// Create some meandres on the river using the shift with random noise
	Heightmap_1D* profile_shift = new Heightmap_1D(800, 0);

	profile_shift->Noise(64 < width / 10 ? 64 : width / 10, height / 8, m_std_noise);

	profile_shift->ScaleValuesTo(-width / 11, width / 11);

	base->Shift(profile_shift, PT_VERTICAL, PT_DISCARD_AND_FILL);

	base->TransformValues(m_natural_profile, true);
}

void Heightmap::LoadLake()
{
	// load map parameters
	int32 width = m_Width;
	int32 height = m_Height;

	// create a radial gradient with height 1 in the center and height 1200 on the outer rim
	base = new Heightmap_2D(width, height, 0);

	base->RadialGradient(width / 2, height / 2, width > height ? height / 2 : width / 2, 1, 1200, true);

	// create a separate noise map using default noise settings
	Heightmap_2D* noise = new Heightmap_2D(width, height, 0);

	noise->Noise(2, width > height ? height / 8 : width / 8, m_std_noise);

	// adjust the range of the noise
	noise->ScaleValuesTo(-500, 500);

	// combine the maps
	base->AddMap(noise);

	// raise the water level so 9% of the map is under level 0
	base->Flood(0.91);

}

void Heightmap::LoadBasic()
{
	int32 width = m_Width;
	int32 height = m_Height;
	int32 water_distribution = 2;
	int32 smoothness = 1 << 1;
	int32 feature_size = 1;
	int32 water_level = 60 / 100.;

	base = new Heightmap_2D(width, height, 0);

	base->Noise(smoothness, ((width > height) ? height : width) / (5 * (4 - feature_size)), m_std_noise);

	if (water_distribution == 1) base->Abs();

	base->Flood(1 - water_level);

	base->TransformValues(m_natural_profile, true);
}

void Heightmap::LoadArchipelago()
{
	int32 width = m_Width;
	int32 height = m_Height;
	int32 smoothness = 1 << 1;
	int32 archipelago_size = 5;
	int32 island_size = 5;

	// create a base archipelago layout
	base = new Heightmap_2D(width, height, 0);

	base->Noise(width / 10, width / (8 - archipelago_size), m_std_noise);
	base->ScaleValuesTo(PT_MIN_HEIGHT / 2, PT_MAX_HEIGHT / 2);
	base->Clamp(PT_MIN_HEIGHT / 16, PT_MAX_HEIGHT / 16);
	base->Add(-5 * PT_MAX_HEIGHT / 36);
	base->Smooth(width / 20);

	// add smaller islands on top of the base shape
	Heightmap_2D* islands = new Heightmap_2D(width, height, 0);

	islands->Noise(smoothness, width / (60 - 8 * island_size), m_std_noise);
	islands->ScaleValuesTo(0, PT_MAX_HEIGHT / 8);

	base->AddMap(islands);

	base->TransformValues(m_natural_profile, true);
}

signed short Heightmap::Sample(unsigned int x, unsigned int y)
{
	return base->GetValue(x, y);
}