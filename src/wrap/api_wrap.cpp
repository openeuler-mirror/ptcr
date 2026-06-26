/******************************************************************************
 * Copyright (c) KylinSoft  Co., Ltd. 2021. All rights reserved.
 * ptcr licensed under the Mulan PSL v2.

 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: xiapin
 * Create: 2021-11-29
 * Description: measure by cri interface implement.
 ******************************************************************************/
#ifdef CRI_CLIENT
#include "api_wrap.h"
#include <stdlib.h>
#include "utils.h"

std::string RuntimeClient::GetVersion(const string &version)
{
    runtime::v1alpha2::VersionRequest request;
    runtime::v1alpha2::VersionResponse response;
    ClientContext context; // used to convey extra information

    request.set_version(version);
    Status status = stub_->Version(&context, request, &response);
    if (status.ok()) {
        return response.version();
    }

    std::cout << status.error_code() << ": " << status.error_message() << endl;
    return "RPC failed";
}

PodSandboxConfig *GenerateSandboxConfig()
{
    /* need to be saved */
    PodSandboxMetadata *metaData;
    LinuxPodSandboxConfig *LinuxPodSbCfg;
    PodSandboxConfig *sdbConifg;

    metaData = new PodSandboxMetadata; /* need save */
    metaData->Clear();
    metaData->set_name("nginx-sandbox");
    metaData->set_namespace_("default");
    metaData->set_attempt(1);

    char *uid = utils_generate_random_str(26);
    metaData->set_uid(uid);

    sdbConifg = new PodSandboxConfig;
    sdbConifg->Clear();
    sdbConifg->set_allocated_metadata(metaData); /* Allocated means we need allocate data first */
    sdbConifg->set_log_directory("/tmp/ptcr_cri.log");

    LinuxPodSbCfg = new LinuxPodSandboxConfig;
    LinuxPodSbCfg->Clear();
    sdbConifg->set_allocated_linux(LinuxPodSbCfg);

    return sdbConifg;
}

int RuntimeClient::RunPodSandBox(string *podID)
{
    NULL_PTR_CHECK(podID, ERR_INVALID_INPUT_PARAM);

    ClientContext context;
    RunPodSandboxRequest request;
    RunPodSandboxResponse response;

    request.set_allocated_config(GenerateSandboxConfig());
    Status status = stub_->RunPodSandbox(&context, request, &response);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    *podID = response.pod_sandbox_id();
    return 0;
}

int RuntimeClient::StopPodSandBox(string &podID)
{
    ClientContext context;
    StopPodSandboxRequest request;
    StopPodSandboxResponse reply;

    request.set_pod_sandbox_id(podID);
    Status status = stub_->StopPodSandbox(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

int RuntimeClient::RemovePodSandBox(string &podID)
{
    ClientContext context;
    RemovePodSandboxRequest request;
    RemovePodSandboxResponse reply;

    request.set_pod_sandbox_id(podID);
    Status status = stub_->RemovePodSandbox(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

int RuntimeClient::CreateContainer
(string &podID, string &imageName, string *contID, bool bRun, vector<string> &cmd)
{
    ClientContext context;
    CreateContainerRequest request;
    CreateContainerResponse reply;

    ContainerConfig *containerCfg = new ContainerConfig;
    ImageSpec *imageSpec = new ImageSpec;
    ContainerMetadata *contMetadata = new ContainerMetadata;
    char *contName = utils_generate_random_str(4);

    imageSpec->set_image(imageName);
    containerCfg->Clear();
    containerCfg->set_allocated_image(imageSpec);

    if (bRun) {
        for (auto it : cmd) {
            containerCfg->add_command(it);
        }
    }

    contMetadata->Clear();
    contMetadata->set_name(contName);
    containerCfg->set_allocated_metadata(contMetadata);
    free(contName);

    request.set_pod_sandbox_id(podID);
    request.set_allocated_config(containerCfg);
    /* The allocated variable released by grpc, guess, need keep the same sdb config */
    request.set_allocated_sandbox_config(GenerateSandboxConfig());

    Status status = stub_->CreateContainer(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }
    *contID = reply.container_id();

    return StartContainer(*contID);
}

int RuntimeClient::StartContainer(string &contID)
{
    ClientContext context;
    StartContainerRequest request;
    StartContainerResponse reply;

    request.set_container_id(contID);
    Status status = stub_->StartContainer(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

int RuntimeClient::StopContainer(string &contID, int timeOut)
{
    ClientContext context;
    StopContainerRequest request;
    StopContainerResponse reply;

    request.set_container_id(contID);
    request.set_timeout(timeOut);

    Status status = stub_->StopContainer(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

int RuntimeClient::RemoveContainer(string &contID)
{
    ClientContext context;
    RemoveContainerRequest request;
    RemoveContainerResponse reply;

    request.set_container_id(contID);
    Status status = stub_->RemoveContainer(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

int ImageClient::PullImage(string &imageName)
{
    ClientContext context;
    PullImageRequest request;
    PullImageResponse reply;
    ImageSpec *imgSpec = new ImageSpec;

    imgSpec->set_image(imageName);
    request.set_allocated_sandbox_config(GenerateSandboxConfig());
    request.set_allocated_image(imgSpec);
    Status status = stub_->PullImage(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

int ImageClient::RemoveImage(string &imageName)
{
    ClientContext context;
    RemoveImageRequest request;
    RemoveImageResponse reply;
    ImageSpec *imgSpec = new ImageSpec;

    imgSpec->set_image(imageName);
    request.set_allocated_image(imgSpec);
    Status status = stub_->RemoveImage(&context, request, &reply);
    if (!status.ok()) {
        LOG_ERROR("err_code:%d: %s\n", status.error_code(), status.error_message().c_str());
        return -1;
    }

    return 0;
}

ApiWrapperCls::ApiWrapperCls(string &cliName) : m_cliName(cliName)
{
    m_criClient = new RuntimeClient(grpc::CreateChannel(m_cliName, grpc::InsecureChannelCredentials()));
    m_criClient->RunPodSandBox(&m_podID);
    m_imageClient = new ImageClient(grpc::CreateChannel(m_cliName, grpc::InsecureChannelCredentials()));
}

ApiWrapperCls::~ApiWrapperCls()
{
    m_criClient->StopPodSandBox(m_podID);
    m_criClient->RemovePodSandBox(m_podID);

    delete m_criClient;
    delete m_imageClient;
}

int ApiWrapperCls::pullImage(string &imageName)
{
    return m_imageClient->PullImage(imageName);
}

int ApiWrapperCls::removeImage(string &imageName)
{
    return m_imageClient->RemoveImage(imageName);
}

int ApiWrapperCls::createContainer(string &imageName, string *contStr)
{
    std::vector<std::string> cmd;
    int ret = m_criClient->CreateContainer(m_podID, imageName, contStr, false, cmd);
    LOG_DEBUG("Create cont:%s ret:%d\n", contStr->c_str(), ret);

    return ret;
}

int ApiWrapperCls::startContainer(string &contStr)
{
    int ret = m_criClient->StartContainer(contStr);
    LOG_DEBUG("Start cont:%s ret:%d\n", contStr.c_str(), ret);

    return ret;
}

int ApiWrapperCls::runContainer(string &imageName, vector<string> &cmd, string *contStr)
{
    return m_criClient->CreateContainer(m_podID, imageName, contStr, true, cmd);
}

int ApiWrapperCls::stopContainer(string contId, int timeOut)
{
    int force = (timeOut < 0) ? 0 : timeOut;
    int ret = m_criClient->StopContainer(contId, force);
    LOG_DEBUG("Stop cont:%s ret:%d\n", contId.c_str(), ret);

    return ret;
}

int ApiWrapperCls::stopAllContainer()
{
    // TODO
    return 0;
}

int ApiWrapperCls::rmContainer(string ContId)
{
    int ret = m_criClient->RemoveContainer(ContId);
    LOG_DEBUG("Remove cont:%s ret:%d\n", ContId.c_str(), ret);

    return ret;
}
#endif // #ifdef CRI_CLIENT
