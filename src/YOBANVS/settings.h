#pragma once

#include "stream.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_timer.h>
#include <esp_log.h>
#include <memory>

#include "nvs_flash.h"

namespace YOBA {
	class NVSSettings {
		public:
			virtual ~NVSSettings() = default;

			static void setup() {
				const auto status = nvs_flash_init();

				if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
					// NVS partition was truncated and needs to be erased
					ESP_ERROR_CHECK(nvs_flash_erase());
					// Retry init
					ESP_ERROR_CHECK(nvs_flash_init());
				}
				else {
					ESP_ERROR_CHECK(status);
				}
			}

			void read() {
				NVSStream stream {};
				stream.openForReading(getNVSNamespace());

				onRead(stream);

				stream.close();
			}

			void write() {
				ESP_LOGI("NVSSettings", "Writing %s", getNVSNamespace());

				NVSStream stream {};
				stream.openForWriting(getNVSNamespace());

				onWrite(stream);

				stream.commit();
				stream.close();
			}

			void scheduleWrite() {
				const auto alreadyScheduled = _scheduledWriteTimeUs > 0;

				_scheduledWriteTimeUs = esp_timer_get_time() + 2'500'000;

				if (alreadyScheduled)
					return;

				xTaskCreate(
					[](void* arg) {
						const auto instance = static_cast<NVSSettings*>(arg);

						while (true) {
							const auto time = esp_timer_get_time();

							if (time >= instance->_scheduledWriteTimeUs)
								break;

							vTaskDelay(pdMS_TO_TICKS((instance->_scheduledWriteTimeUs - time) / 1000));
						}

						instance->_scheduledWriteTimeUs = 0;
						instance->write();

						vTaskDelete(nullptr);
					},
					"NVSSerWrite",
					4096,
					this,
					1,
					nullptr
				);
			}

		protected:
			virtual const char* getNVSNamespace() = 0;
			virtual void onRead(const NVSStream& stream) = 0;
			virtual void onWrite(const NVSStream& stream) = 0;

		private:
			constexpr static uint32_t _writeDelayTicks = pdMS_TO_TICKS(2500);
			uint32_t _scheduledWriteTimeUs = 0;
	};
}