/*
 * Task.cpp
 *
 *  Created on: Mar 4, 2017
 *      Author: kolban
 */

#include "Task.h"

#include <esp_log.h>

#include <string>

#include "sdkconfig.h"

static const char* LOG_TAG = "Task";

/**
 * @brief Create an instance of the task class.
 *
 * @param [in] taskName The name of the task to create.
 * @param [in] stackSize The size of the stack.
 * @return N/A.
 */
Task::Task(std::string taskName, uint16_t stackSize, uint8_t priority) {
  m_taskName = taskName;
  m_stackSize = stackSize;
  m_priority = priority;
  m_taskData = nullptr;
  m_handle = nullptr;
  m_coreId = tskNO_AFFINITY;
}  // Task

Task::~Task() { stop(); }  // ~Task

/**
 * @brief Suspend the task for the specified milliseconds.
 *
 * @param [in] ms The delay time in milliseconds.
 * @return N/A.
 */

/* static */ void Task::delay(int ms) { ::vTaskDelay(ms / portTICK_PERIOD_MS); }  // delay

/**
 * Static class member that actually runs the target task.
 *
 * The code here will run on the task thread.
 * @param [in] pTaskInstance The task to run.
 */
void Task::runTask(void* pTaskInstance) {
  Task* pTask = (Task*)pTaskInstance;
  ESP_LOGD(LOG_TAG, ">> runTask: taskName=%s", pTask->m_taskName.c_str());
  pTask->run(pTask->m_taskData);
  ESP_LOGD(LOG_TAG, "<< runTask: taskName=%s", pTask->m_taskName.c_str());
  pTask->stop();
}  // runTask

/**
 * @brief Start an instance of the task.
 *
 * @param [in] taskData Data to be passed into the task.
 * @return N/A.
 */
void Task::start(void* taskData) {
  if (m_handle != nullptr) {
    ESP_LOGW(LOG_TAG, "Task::start - There might be a task already running!");
  }
  m_taskData = taskData;
  ::xTaskCreatePinnedToCore(&runTask, m_taskName.c_str(), m_stackSize, this, m_priority, &m_handle, m_coreId);
}  // start

/**
 * @brief Stop the task.
 *
 * @return N/A.
 */
void Task::stop() {
  if (m_handle == nullptr) return;
  TaskHandle_t temp = m_handle;
  m_handle = nullptr;
  ::vTaskDelete(temp);
}  // stop

/**
 * @brief Set the stack size of the task.
 *
 * @param [in] stackSize The size of the stack for the task.
 * @return N/A.
 */
void Task::setStackSize(uint16_t stackSize) { m_stackSize = stackSize; }  // setStackSize

/**
 * @brief Set the priority of the task.
 *
 * @param [in] priority The priority for the task.
 * @return N/A.
 */
void Task::setPriority(uint8_t priority) { m_priority = priority; }  // setPriority

/**
 * @brief Set the name of the task.
 *
 * @param [in] name The name for the task.
 * @return N/A.
 */
void Task::setName(std::string name) { m_taskName = name; }  // setName

/**
 * @brief Set the core number the task has to be executed on.
 * If the core number is not set, tskNO_AFFINITY will be used
 *
 * @param [in] coreId The id of the core.
 * @return N/A.
 */
void Task::setCore(BaseType_t coreId) { m_coreId = coreId; }
