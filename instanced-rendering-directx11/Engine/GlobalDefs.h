#pragma once

#define FAIL_CHECK if (FAILED(hr)) return hr;
#define RELEASE_RESOURCE(x) if (x) (x)->Release();
#define RAND(MIN, MAX) (float)((MIN) + rand() / (RAND_MAX / ((MAX) - (MIN) + 1) + 1))

constexpr float LOD_1 = 100.0f;
constexpr float LOD_2 = 250.0f;