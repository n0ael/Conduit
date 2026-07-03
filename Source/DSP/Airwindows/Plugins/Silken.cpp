/* ========================================
 *  Silken - Silken.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Silken —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Silken.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "silken", "Silken", 0.0f },
        { "freq",   "Freq",   0.5f },
        { "window", "Window", 0.5f },
    };

    // Original-Tabelle (statisch, unveraendert): 1000 Ganzzahl-Offsets fuer
    // die Sinc-Abtastung, siehe Airwindows Silken.h.
    constexpr int kUnprime[1000] = {
        1, 2, 4, 6, 8, 9, 10, 12, 14, 15, 16, 18, 20, 21, 22, 24, 25, 26, 27, 28,
        30, 32, 33, 34, 35, 36, 38, 39, 40, 42, 44, 45, 46, 48, 49, 50, 51, 52, 54, 55,
        56, 57, 58, 60, 62, 63, 64, 65, 66, 68, 69, 70, 72, 74, 75, 76, 77, 78, 80, 81,
        82, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 98, 99, 100, 102, 104, 105, 106,
        108, 110, 111, 112, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 128, 129, 130,
        132, 133, 134, 135, 136, 138, 140, 141, 142, 143, 144, 145, 146, 147, 148, 150, 152, 153, 154, 155,
        156, 158, 159, 160, 161, 162, 164, 165, 166, 168, 169, 170, 171, 172, 174, 175, 176, 177, 178, 180,
        182, 183, 184, 185, 186, 187, 188, 189, 190, 192, 194, 195, 196, 198, 200, 201, 202, 203, 204, 205,
        206, 207, 208, 209, 210, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 224, 225, 226, 228,
        230, 231, 232, 234, 235, 236, 237, 238, 240, 242, 243, 244, 245, 246, 247, 248, 249, 250, 252, 253,
        254, 255, 256, 258, 259, 260, 261, 262, 264, 265, 266, 267, 268, 270, 272, 273, 274, 275, 276, 278,
        279, 280, 282, 284, 285, 286, 287, 288, 289, 290, 291, 292, 294, 295, 296, 297, 298, 299, 300, 301,
        302, 303, 304, 305, 306, 308, 309, 310, 312, 314, 315, 316, 318, 319, 320, 321, 322, 323, 324, 325,
        326, 327, 328, 329, 330, 332, 333, 334, 335, 336, 338, 339, 340, 341, 342, 343, 344, 345, 346, 348,
        350, 351, 352, 354, 355, 356, 357, 358, 360, 361, 362, 363, 364, 365, 366, 368, 369, 370, 371, 372,
        374, 375, 376, 377, 378, 380, 381, 382, 384, 385, 386, 387, 388, 390, 391, 392, 393, 394, 395, 396,
        398, 399, 400, 402, 403, 404, 405, 406, 407, 408, 410, 411, 412, 413, 414, 415, 416, 417, 418, 420,
        422, 423, 424, 425, 426, 427, 428, 429, 430, 432, 434, 435, 436, 437, 438, 440, 441, 442, 444, 445,
        446, 447, 448, 450, 451, 452, 453, 454, 455, 456, 458, 459, 460, 462, 464, 465, 466, 468, 469, 470,
        471, 472, 473, 474, 475, 476, 477, 478, 480, 481, 482, 483, 484, 485, 486, 488, 489, 490, 492, 493,
        494, 495, 496, 497, 498, 500, 501, 502, 504, 505, 506, 507, 508, 510, 511, 512, 513, 514, 515, 516,
        517, 518, 519, 520, 522, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538,
        539, 540, 542, 543, 544, 545, 546, 548, 549, 550, 551, 552, 553, 554, 555, 556, 558, 559, 560, 561,
        562, 564, 565, 566, 567, 568, 570, 572, 573, 574, 575, 576, 578, 579, 580, 581, 582, 583, 584, 585,
        586, 588, 589, 590, 591, 592, 594, 595, 596, 597, 598, 600, 602, 603, 604, 605, 606, 608, 609, 610,
        611, 612, 614, 615, 616, 618, 620, 621, 622, 623, 624, 625, 626, 627, 628, 629, 630, 632, 633, 634,
        635, 636, 637, 638, 639, 640, 642, 644, 645, 646, 648, 649, 650, 651, 652, 654, 655, 656, 657, 658,
        660, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671, 672, 674, 675, 676, 678, 679, 680, 681, 682,
        684, 685, 686, 687, 688, 689, 690, 692, 693, 694, 695, 696, 697, 698, 699, 700, 702, 703, 704, 705,
        706, 707, 708, 710, 711, 712, 713, 714, 715, 716, 717, 718, 720, 721, 722, 723, 724, 725, 726, 728,
        729, 730, 731, 732, 734, 735, 736, 737, 738, 740, 741, 742, 744, 745, 746, 747, 748, 749, 750, 752,
        753, 754, 755, 756, 758, 759, 760, 762, 763, 764, 765, 766, 767, 768, 770, 771, 772, 774, 775, 776,
        777, 778, 779, 780, 781, 782, 783, 784, 785, 786, 788, 789, 790, 791, 792, 793, 794, 795, 796, 798,
        799, 800, 801, 802, 803, 804, 805, 806, 807, 808, 810, 812, 813, 814, 815, 816, 817, 818, 819, 820,
        822, 824, 825, 826, 828, 830, 831, 832, 833, 834, 835, 836, 837, 838, 840, 841, 842, 843, 844, 845,
        846, 847, 848, 849, 850, 851, 852, 854, 855, 856, 858, 860, 861, 862, 864, 865, 866, 867, 868, 869,
        870, 871, 872, 873, 874, 875, 876, 878, 879, 880, 882, 884, 885, 886, 888, 889, 890, 891, 892, 893,
        894, 895, 896, 897, 898, 899, 900, 901, 902, 903, 904, 905, 906, 908, 909, 910, 912, 913, 914, 915,
        916, 917, 918, 920, 921, 922, 923, 924, 925, 926, 927, 928, 930, 931, 932, 933, 934, 935, 936, 938,
        939, 940, 942, 943, 944, 945, 946, 948, 949, 950, 951, 952, 954, 955, 956, 957, 958, 959, 960, 961,
        962, 963, 964, 965, 966, 968, 969, 970, 972, 973, 974, 975, 976, 978, 979, 980, 981, 982, 984, 985,
        986, 987, 988, 989, 990, 992, 993, 994, 995, 996, 998, 999, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007,
        1008, 1010, 1011, 1012, 1014, 1015, 1016, 1017, 1018, 1020, 1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 1032,
        1034, 1035, 1036, 1037, 1038, 1040, 1041, 1042, 1043, 1044, 1045, 1046, 1047, 1048, 1050, 1052, 1053, 1054, 1055, 1056,
        1057, 1058, 1059, 1060, 1062, 1064, 1065, 1066, 1067, 1068, 1070, 1071, 1072, 1073, 1074, 1075, 1076, 1077, 1078, 1079,
        1080, 1081, 1082, 1083, 1084, 1085, 1086, 1088, 1089, 1090, 1092, 1094, 1095, 1096, 1098, 1099, 1100, 1101, 1102, 1104,
        1105, 1106, 1107, 1108, 1110, 1111, 1112, 1113, 1114, 1115, 1116, 1118, 1119, 1120, 1121, 1122, 1124, 1125, 1126, 1127,
        1128, 1130, 1131, 1132, 1133, 1134, 1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148,
        1149, 1150, 1152, 1154, 1155, 1156, 1157, 1158, 1159, 1160, 1161, 1162, 1164, 1165, 1166, 1167, 1168, 1169, 1170, 1172,
        1173, 1174, 1175, 1176, 1177, 1178, 1179, 1180, 1182, 1183, 1184, 1185, 1186, 1188, 1189, 1190, 1191, 1192, 1194, 1195,
    };
}

Silken::Silken() noexcept
    : AirwindowsPlugin ("silken", "Silken", kParameters)
{
}

void Silken::resetState() noexcept
{
    for (auto& v : firBufferL) v = 0.0;
    for (auto& v : firBufferR) v = 0.0;
    for (auto& v : fir) v = 0.0;
    firPosition = 0;
    firlastSampleL = 0.0;
    firlastSampleR = 0.0;
    for (auto& v : infirmediateL) v = 0.0;
    for (auto& v : infirmediateR) v = 0.0;
}

void Silken::processStereo (const float* in1, const float* in2,
                           float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();
    int spacing = (int) std::floor (overallscale); //should give us working basic scaling, usually 2 or 4
    if (spacing < 1) spacing = 1; if (spacing > 16) spacing = 16;

    double wet = A;
    double freq = std::pow (B, 2) * juce::MathConstants<double>::halfPi; if (freq < 0.0001) freq = 0.0001;
    double positionMiddle = std::sin (freq) * 0.5; //shift relative to frequency, not sample-rate
    freq /= overallscale; //generating the FIR relative to sample rate
    const int window = (int) std::fmin ((C * C * 256.0 * overallscale) + 2.0, 998.0); //so's the window size
    const int middle = (int) ((double) window * positionMiddle);
    for (int fip = 0; fip < middle; fip++)
    {
        fir[fip] = (kUnprime[middle - fip]) * freq;
        fir[fip] = std::sin (fir[fip]) / fir[fip]; //sinc function
        fir[fip] *= std::sin (((double) fip / (double) window) * juce::MathConstants<double>::pi); //windowed with sin()
    }
    fir[middle] = 1.0;
    for (int fip = middle + 1; fip < window; fip++)
    {
        fir[fip] = (kUnprime[fip - middle]) * freq;
        fir[fip] = std::sin (fir[fip]) / fir[fip]; //sinc function
        fir[fip] *= std::sin (((double) fip / (double) window) * juce::MathConstants<double>::pi); //windowed with sin()
    }

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        if (firPosition < 0 || firPosition > 32767) firPosition = 32767;
        int firp = firPosition;
        firBufferL[firp] = inputSampleL; inputSampleL = 0.0;
        firBufferR[firp] = inputSampleR; inputSampleR = 0.0;

        if (firp + kUnprime[window] < 32767)
        {
            for (int fip = 1; fip < window; fip++)
            {
                inputSampleL += firBufferL[firp + kUnprime[fip]] * fir[fip];
                inputSampleR += firBufferR[firp + kUnprime[fip]] * fir[fip];
            }
        }
        else
        {
            for (int fip = 1; fip < window; fip++)
            {
                inputSampleL += firBufferL[firp + kUnprime[fip] - ((firp + kUnprime[fip] > 32767) ? 32768 : 0)] * fir[fip];
                inputSampleR += firBufferR[firp + kUnprime[fip] - ((firp + kUnprime[fip] > 32767) ? 32768 : 0)] * fir[fip];
            }
        }
        inputSampleL *= std::sqrt (freq * 0.618033988749894848204586); //compensate for gain
        inputSampleR *= std::sqrt (freq * 0.618033988749894848204586); //compensate for gain
        firPosition--;

        double softSpeed = std::fabs (inputSampleL);
        if (softSpeed < 1.0) softSpeed = 1.0; else softSpeed = 1.0 / softSpeed;
        inputSampleL = std::sin (inputSampleL) * 0.9549925859; //scale to what cliponly uses
        inputSampleL = (inputSampleL * softSpeed) + (firlastSampleL * (1.0 - softSpeed));

        softSpeed = std::fabs (inputSampleR);
        if (softSpeed < 1.0) softSpeed = 1.0; else softSpeed = 1.0 / softSpeed;
        inputSampleR = std::sin (inputSampleR) * 0.9549925859; //scale to what cliponly uses
        inputSampleR = (inputSampleR * softSpeed) + (firlastSampleR * (1.0 - softSpeed));

        infirmediateL[spacing] = inputSampleL;
        inputSampleL = firlastSampleL; //Latency is however many samples equals one 44.1k sample
        for (int x = spacing; x > 0; x--) infirmediateL[x - 1] = infirmediateL[x];
        firlastSampleL = infirmediateL[0]; //run a little buffer to handle this

        infirmediateR[spacing] = inputSampleR;
        inputSampleR = firlastSampleR; //Latency is however many samples equals one 44.1k sample
        for (int x = spacing; x > 0; x--) infirmediateR[x - 1] = infirmediateR[x];
        firlastSampleR = infirmediateR[0]; //run a little buffer to handle this

        if (firp + kUnprime[middle] < 32768)
        {
            inputSampleL = (firBufferL[firp + kUnprime[middle]] * (wet + 1.0)) - (inputSampleL * wet);
            inputSampleR = (firBufferR[firp + kUnprime[middle]] * (wet + 1.0)) - (inputSampleR * wet);
        }
        else
        {
            inputSampleL = (firBufferL[firp + kUnprime[middle] - 32768] * (wet + 1.0)) - (inputSampleL * wet);
            inputSampleR = (firBufferR[firp + kUnprime[middle] - 32768] * (wet + 1.0)) - (inputSampleR * wet);
        } //dry/wet must use a sample from the middle of firBuffer for dry,
        //because it's an FIR filter that is phase linear by nature

        //begin 32 bit stereo floating point dither
        if (ditherOn())
        {
            int expon;
            std::frexp ((float) inputSampleL, &expon);
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            inputSampleL += (double) ((double (fpdL) - std::uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
            std::frexp ((float) inputSampleR, &expon);
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
            inputSampleR += (double) ((double (fpdR) - std::uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
        }
        else
        {
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
        }
        //end 32 bit stereo floating point dither

        *out1 = (float) inputSampleL;
        *out2 = (float) inputSampleR;

        ++in1;
        ++in2;
        ++out1;
        ++out2;
    }
}

} // namespace conduit::airwindows
