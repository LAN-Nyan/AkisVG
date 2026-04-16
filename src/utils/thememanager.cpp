#include "thememanager.h"
#include <QSettings>

// Cached scale factor — -1 means "not yet read".
static double s_uiScaleCache = -1.0;

double uiScale()
{
    if (s_uiScaleCache < 0) {
        QSettings st("AkisVG", "AkisVG");
        int pct = st.value("ui/scale", 100).toInt();
        // Clamp to the valid range defined in the slider (50–200)
        pct = qBound(50, pct, 200);
        s_uiScaleCache = pct / 100.0;
    }
    return s_uiScaleCache;
}

void invalidateUiScaleCache()
{
    s_uiScaleCache = -1.0;
}
