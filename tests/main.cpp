#include <QtTest>
#include "placement_tests.h"
#include "placement_assist_tests.h"
#include "inventory_safety_tests.h"
#include "shape_manager_tests.h"
#include "stm_safety_tests.h"

int main(int argc, char **argv)
{
    int status = 0;
    {
        PlacementTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        PlacementAssistTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        InventorySafetyTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        ShapeManagerTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        StmSafetyTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
