
// File including several others...
#pragma once

<uno>
#include <inc_test_1.h>
</uno>

<dos>
#include <inc_test_2.h>
</dos>

<tres>
// This one is already included from inc_test_2, so it won't be expanded again in here.
#include <inc_test_3.h>
</tres>

<quatro>
"The end";
</quatro>

