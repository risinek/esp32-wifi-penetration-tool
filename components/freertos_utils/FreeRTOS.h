/*
 * FreeRTOS.h
 *
 *  Created on: Feb 24, 2017
 *      Author: kolban
 */

#ifndef MAIN_FREERTOS_H_
#define MAIN_FREERTOS_H_
#include <pthread.h>
#include <stdint.h>

#include <string>

#include "freertos/FreeRTOS.h"  // Include the base FreeRTOS definitions.
#include "freertos/ringbuf.h"   // Include the ringbuffer definitions.
#include "freertos/semphr.h"    // Include the semaphore definitions.
#include "freertos/task.h"      // Include the task definitions.

/**
 * @brief Interface to %FreeRTOS functions.
 */
class FreeRTOS {
 public:
  static void sleep(uint32_t ms);
  static void startTask(void task(void*), std::string taskName, void* param = nullptr, uint32_t stackSize = 2048);
  static void deleteTask(TaskHandle_t pTask = nullptr);

  static uint32_t getTimeSinceStart();

  class Semaphore {
   public:
    Semaphore(std::string owner = "<Unknown>");
    ~Semaphore();
    void give();
    void give(uint32_t value);
    void giveFromISR();
    void setName(std::string name);
    bool take(std::string owner = "<Unknown>");
    bool take(uint32_t timeoutMs, std::string owner = "<Unknown>");
    std::string toString();
    uint32_t wait(std::string owner = "<Unknown>");

   private:
    SemaphoreHandle_t m_semaphore;
    pthread_mutex_t m_pthread_mutex;
    std::string m_name;
    std::string m_owner;
    uint32_t m_value;
    bool m_usePthreads;
  };
};

#endif /* MAIN_FREERTOS_H_ */
