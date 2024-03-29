#include <string.h>

#include <unistd.h>
#include <pwd.h>
#include <iostream>
#include "sgx_urts.h"
#include "Enclave_u.h"
#include "LibSgxExample.h"

#include "LibSgxJava_SgxFunction.h"
#include <jni.h>

#define TMC_SIZE 580
#define MAX_PATH FILENAME_MAX

/* EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;
sgx_status_t ret = SGX_SUCCESS;
sgx_launch_token_t token = { 0 };
int updated = 0;
using namespace std;
uint32_t enc_ret;
uint8_t seal[TMC_SIZE];

/* Function: Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
JNIEXPORT jint JNICALL Java_LibSgxJava_SgxFunction_jni_1initialize_1enclave
        (JNIEnv * env, jobject obj, jint idi)
{
        char id[1];
        id[0] = (idi) + 0x30;
        char token_path[MAX_PATH] = {'\0'};
        sgx_launch_token_t token = {0};
        sgx_status_t ret = SGX_ERROR_UNEXPECTED;
        int updated = 0;

        printf("Final Intel SGX monotonic Counter  v:2.0.1\n");
        /* Step 1: try to retrieve the launch token saved by last transaction
         *         if there is no token, then create a new one.
         */
        /* try to get the token saved in $HOME */
        const char *home_dir = getpwuid(getuid())->pw_dir;

        if (home_dir != NULL &&
            (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
                /* compose the token path */
                strncpy(token_path, home_dir, strlen(home_dir));
                strncat(token_path, "/", strlen("/"));
                strncat(token_path, id, sizeof(char)+1);
                strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
        } else {
                /* if token path is too long or $HOME is NULL */
                strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
        }

        printf("Launch token file \"%s\".\n", token_path);
        FILE *fp = fopen(token_path, "rb");
        if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
                printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
        }

        if (fp != NULL) {
                /* read the token from saved file */
                size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
                if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
                        /* if token is invalid, clear the buffer */
                        memset(&token, 0x0, sizeof(sgx_launch_token_t));
                        printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
                }
        }
        /* Step 2: call sgx_create_enclave to initialize an enclave instance */
        /* Debug Support: set 2nd parameter to 1 */
        ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
        if (ret != SGX_SUCCESS) {
                if (fp != NULL) fclose(fp);
                return -1;
        }

        /* Step 3: save the launch token if it is updated */
        if (updated == FALSE || fp == NULL) {
                /* if the token is not updated, or file handler is invalid, do not perform saving */
                if (fp != NULL) fclose(fp);
                return 0;
        }

        /* reopen the file with write capablity */
        fp = freopen(token_path, "wb", fp);
        if (fp == NULL) return 0;
        size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
        if (write_num != sizeof(sgx_launch_token_t))
                printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
        fclose(fp);
        return 0;
}

/* Function: Destroy the enclave */
JNIEXPORT void JNICALL Java_LibSgxJava_SgxFunction_jni_1sgx_1destroy_1enclave
        (JNIEnv * env, jobject obj)
{
        sgx_status_t ret = SGX_ERROR_UNEXPECTED;
        ret = sgx_destroy_enclave(global_eid);
        if (ret != SGX_SUCCESS) {
                printf("Failed to destroy eclave: INVALID_ENCLAVE_ID.\n");
        }
}


/* Function: Create monotonic counter */
JNIEXPORT void JNICALL Java_LibSgxJava_SgxFunction_jni_1ecall_1sgx_1create_1counter
        (JNIEnv * env, jobject obj)
{
        ret = create_sealed_monotonic_counter(global_eid, &enc_ret, seal, TMC_SIZE);
        if (ret != SGX_SUCCESS || enc_ret != SGX_SUCCESS) {
                cerr << "Failed to create monotonic counter" << endl;
        }
}

/* Function: Read monotonic counter */
JNIEXPORT jint JNICALL Java_LibSgxJava_SgxFunction_jni_1ecall_1sgx_1read_1counter
        (JNIEnv * env, jobject obj)
{
        uint32_t ctr;
        uint32_t value = 0;

        ret = read_sealed_monotonic_counter(global_eid, &enc_ret, seal, TMC_SIZE, &value);
        if (ret != SGX_SUCCESS || enc_ret != SGX_SUCCESS) {
                cerr << "Failed to read monotonic counter" << endl;
        }
        return value;
}

/* Function: Increment monotonic counter */
JNIEXPORT void JNICALL Java_LibSgxJava_SgxFunction_jni_1ecall_1sgx_1increment_1counter
        (JNIEnv * env, jobject obj, jlong ptr)
{
        ret = increment_monotonic_counter(global_eid, &enc_ret, seal, TMC_SIZE);
        if (ret != SGX_SUCCESS || enc_ret != SGX_SUCCESS) {
                cerr << "Failed to increment monotonic counter" << endl;
        }
}
