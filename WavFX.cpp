// WavFX.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

// Standard includes
#include <iostream>
#include <chrono>

// Custom includes
#include "WavPlayer.h"
#include "ReverbModel.h"
#include "Binaurality.h"
#include "Graphics.h"
#include "Menu.h"


// Project start
int main()
{
    // Loading audio files
    WavPlayer player("Faded(mono).wav", WavPlayer::getDevices()[0], true);
    // Setting up default values to avoid clipping (make system sound louder)
    player.setDry(0.0f);
    player.setWet(0.1f);

    // Setting up effects
    // Using custom file with HRIR data
    Binaurality bin("Sub1HRIR.bin", player.getFrameSize());
    Freeverb fv;
    player.applyEffect(fv);  // Reverberation
    player.applyEffect(bin); // Binauralization

    // Setting up graphics
    Graphics anime(COORD{ 120, 30 }, player.getFrameSize());

    // Setting up user menu
    Menu menu(player, anime, bin, fv);

    // Start playing
    player.play();

    // Time measurement things
    using namespace std::chrono;
    auto lastTime = high_resolution_clock::now();
    uint64_t deltaTime = 0;

    // Infinitely play and draw
    while (true)
    {
        // Getting some fresh samples for visual FFT
        player.getLastSamples(anime.getSamplesArray());

        // Checking new user input
        menu.updateInput();
        // Updating virtual sound source pos
        anime.setAzimuth(bin.getAzimuth());
        anime.setElevation(bin.getElevation());

        // Drawing new frame
        anime.process(menu, deltaTime);

        // High resolution fps (20)
        do {
            deltaTime = (high_resolution_clock::now() - lastTime).count();
        } while (deltaTime < 50e6/*us = 50ms*/);
        lastTime = high_resolution_clock::now();
    }

    return 0;
}