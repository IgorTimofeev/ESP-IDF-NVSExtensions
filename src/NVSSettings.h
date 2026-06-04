#pragma once

#include "NVSStream.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <esp_timer.h>
#include <esp_log.h>
#include <memory>

namespace YOBA {
	class NVSSettings {
		public:
			virtual ~NVSSettings() = default;

			NVSSettings() {
				_timerHandle = xTimerCreateStatic(
					"NVSSettings",
					pdMS_TO_TICKS(2500),
					pdFALSE,
					this,
					[](const TimerHandle_t timer) {
						const auto instance = static_cast<NVSSettings*>(pvTimerGetTimerID(timer));

						instance->write();
					},
					&_timer
				);
			}

			void read() {
				NVSStream stream {};
				stream.openForReading(getNamespace());

				onRead(stream);

				stream.close();
			}

			void write() {
				ESP_LOGI("NVSSettings", "Writing %s", getNamespace());

				NVSStream stream {};
				stream.openForWriting(getNamespace());

				onWrite(stream);

				stream.commit();
				stream.close();
			}

			void scheduleWrite() const {
				resetTimer();
			}

		protected:
			virtual const char* getNamespace() = 0;
			virtual void onRead(const NVSStream& stream) = 0;
			virtual void onWrite(const NVSStream& stream) = 0;

		private:
			StaticTimer_t _timer {};
			TimerHandle_t _timerHandle = nullptr;

			void resetTimer() const {
				if (xPortInIsrContext() == pdTRUE) {
					auto xHigherPriorityTaskWoken = pdFALSE;

					if (xTimerResetFromISR(_timerHandle, &xHigherPriorityTaskWoken) == pdPASS)
						portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
				}
				else {
					xTimerReset(_timerHandle, 0);
				}
			}
	};
}