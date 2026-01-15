#pragma once
#include <string>
#include "Activity.h"

class GpxExporter {
public: 
    static void ExportToGpx(const Activity& activity, const std::string& outPath);
};