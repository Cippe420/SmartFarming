/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#ifdef __linux__
#include <signal.h>
#include <sys/prctl.h>
#endif

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>
#include <openthread/dataset_ftd.h>
#include <string.h>

#include "openthread-system.h"
#include "cli/cli_config.h"
#include "common/code_utils.hpp"

#include "lib/platform/reset_util.h"

/**
 * Initializes the CLI app.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 */
extern void otAppCliInit(otInstance *aInstance);

#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
OT_TOOL_WEAK void *otPlatCAlloc(size_t aNum, size_t aSize) { return calloc(aNum, aSize); }

OT_TOOL_WEAK void otPlatFree(void *aPtr) { free(aPtr); }
#endif

void otTaskletsSignalPending(otInstance *aInstance) { OT_UNUSED_VARIABLE(aInstance); }


void setNetworkConfiguration(otInstance *aInstance)
{
    static char          aNetworkName[] = "OpenThreadDemo";
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    aDataset.mActiveTimestamp.mSeconds             = 1;
    aDataset.mActiveTimestamp.mTicks               = 0;
    aDataset.mActiveTimestamp.mAuthoritative       = false;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel to 15 */
    aDataset.mChannel                      = 14;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID to 1234 */
    aDataset.mPanId                      = (otPanId)0x1234;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID to C0DE1AB5C0DE1AB5 */
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22};
    memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key to 1234C0DE1AB51234C0DE1AB51234C0DE */
    uint8_t key[OT_NETWORK_KEY_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    /* Set pskc to TMPSENS1*/
    uint8_t pskc[OT_PSKC_MAX_SIZE] = "TMPSENS1";
    memcpy(aDataset.mPskc.m8,pskc,sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsPskcPresent = true;

    /* Set Network Name to OpenThreadDemo */
    size_t length = strlen(aNetworkName);
    assert(length <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    otDatasetSetActive(aInstance, &aDataset);

}


#if OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
static otError ProcessExit(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    exit(EXIT_SUCCESS);
}

#if OPENTHREAD_EXAMPLES_SIMULATION
extern otError ProcessNodeIdFilter(void *aContext, uint8_t aArgsLength, char *aArgs[]);
#endif

static const otCliCommand kCommands[] = {
    {"exit", ProcessExit},
#if OPENTHREAD_EXAMPLES_SIMULATION
    /*
     * The CLI command `nodeidfilter` only works for simulation in real time.
     *
     * It can be used either as an allow list or a deny list. Once the filter is cleared, the first `nodeidfilter allow`
     * or `nodeidfilter deny` will determine whether it is set up as an allow or deny list. Subsequent calls should
     * use the same sub-command to add new node IDs, e.g., if we first call `nodeidfilter allow` (which sets the filter
     * up  as an allow list), a subsequent `nodeidfilter deny` will result in `InvalidState` error.
     *
     * The usage of the command `nodeidfilter`:
     *     - `nodeidfilter deny <nodeid>` :  It denies the connection to a specified node (use as deny-list).
     *     - `nodeidfilter allow <nodeid> :  It allows the connection to a specified node (use as allow-list).
     *     - `nodeidfilter clear`         :  It restores the filter state to default.
     *     - `nodeidfilter`               :  Outputs filter mode (allow-list or deny-list) and filtered node IDs.
     */
    {"nodeidfilter", ProcessNodeIdFilter},
#endif
};
#endif // OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)

int main(int argc, char *argv[])
{
    otInstance *instance;

#ifdef __linux__
    // Ensure we terminate this process if the
    // parent process dies.
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

    OT_SETUP_RESET_JUMP(argv);

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t   otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer       = NULL;
#endif

pseudo_reset:

    otSysInit(argc, argv);

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    // Call to query the buffer size
    (void)otInstanceInit(NULL, &otInstanceBufferLength);

    // Call to allocate the buffer
    otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
    assert(otInstanceBuffer);

    // Initialize OpenThread with the buffer
    instance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
    instance = otInstanceInitSingle();
#endif
    assert(instance);


    otAppCliInit(instance);

    // set network config
    setNetworkConfiguration(instance);

    /* Start the Thread network interface (CLI cmd > ifconfig up) */
    otIp6SetEnabled(instance, true);

    /* Start the Thread stack (CLI cmd > thread start) */
    otThreadSetEnabled(instance, true);


#if OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    IgnoreError(otCliSetUserCommands(kCommands, OT_ARRAY_LENGTH(kCommands), instance));
#endif

#if OPENTHREAD_CONFIG_PLATFORM_LOG_CRASH_DUMP_ENABLE
    IgnoreError(otPlatLogCrashDump());
#endif

    while (!otSysPseudoResetWasRequested())
    {
        otTaskletsProcess(instance);
        otSysProcessDrivers(instance);
    }

    otInstanceFinalize(instance);
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    free(otInstanceBuffer);
#endif

    goto pseudo_reset;

    return 0;
}

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list ap;

    va_start(ap, aFormat);
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif
