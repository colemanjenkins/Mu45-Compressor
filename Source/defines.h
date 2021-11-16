/*
  ==============================================================================

    defines.h
    Created: 13 Nov 2021 11:48:59pm
    Author:  Coleman Jenkins

  ==============================================================================
*/

#pragma once

#define NO_SKEW             1

#define THRESH_MIN          -40.0 // dB
#define THRESH_DEFAULT      -20.0
#define THRESH_MAX          0.0
#define THRESH_INTERVAL     0.1
#define THRESH_SKEW         NO_SKEW

#define RATIO_MIN           1.0 // ratio
#define RATIO_DEFAULT       2.0
#define RATIO_MAX           30.0
#define RATIO_INTERVAL      0.1
#define RATIO_SKEW          0.5

#define ATTACK_MIN          0.0 // ms
#define ATTACK_DEFAULT      20.0
#define ATTACK_MAX          200.0
#define ATTACK_INTERVAL     0.1
#define ATTACK_SKEW         0.5

#define RELEASE_MIN         5.0 // ms
#define RELEASE_DEFAULT     200.0
#define RELEASE_MAX         2000.0
#define RELEASE_INTERVAL    1
#define RELEASE_SKEW        0.5

#define GAIN_MIN            -30.0 // dB
#define GAIN_DEFAULT        0.0
#define GAIN_MAX            30.0
#define GAIN_INTERVAL       0.1
#define GAIN_SKEW           NO_SKEW

// GUI
#define UNIT_LENGTH_X       30
#define UNIT_LENGTH_Y       30

#define CONTAINER_WIDTH     UNIT_LENGTH_X*21
#define CONTAINER_HEIGHT    UNIT_LENGTH_Y*16
