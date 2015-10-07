#include "common.h"

//==================================================================================================
//==================================================================================================
// Variables

static GenericCallback s_update_callback = NULL;


//==================================================================================================
//==================================================================================================
// Interface functions

void common_set_update_callback( GenericCallback callback )
{
	s_update_callback = callback;
}

GenericCallback common_get_update_callback()
{
	return s_update_callback;
}