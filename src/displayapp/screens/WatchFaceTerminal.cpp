#include "WatchFaceTerminal.h"

#include <date/date.h>
#include <lvgl/lvgl.h>
#include <cstdio>
#include "Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "../DisplayApp.h"

using namespace Pinetime::Applications::Screens;

WatchFaceTerminal::WatchFaceTerminal(DisplayApp* app,
                                   Controllers::DateTime& dateTimeController,
                                   Controllers::Battery& batteryController,
                                   Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificatioManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
				   System::SystemTask& systemTask)
  : Screen(app),
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificatioManager {notificatioManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {
  settingsController.SetClockFace(2);

  displayedChar[0] = 0;
  displayedChar[1] = 0;
  displayedChar[2] = 0;
  displayedChar[3] = 0;
  displayedChar[4] = 0;

  prompt_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(prompt_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_obj_align(prompt_date, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 0, 0);
  lv_label_set_text_static(prompt_date, "$ watch -n 60 date");

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 25);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, label_time, LV_ALIGN_OUT_RIGHT_MID, 0, 10);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));

  prompt_status = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(prompt_status, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_obj_align(prompt_status, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -2);
  lv_label_set_text_static(prompt_status, "$ status");

  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text(backgroundLabel, "");

  heartbeatBpm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatBpm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(heartbeatBpm, "BPM");
  lv_obj_align(heartbeatBpm, prompt_status, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text(heartbeatValue, "---");
  lv_obj_align_y(heartbeatValue, heartbeatBpm, LV_ALIGN_OUT_RIGHT_MID, 0);

  label_step = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_step, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(label_step, "Step");
  lv_obj_align(label_step, heartbeatBpm, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text(stepValue, "0");
  lv_obj_align_y(stepValue, label_step, LV_ALIGN_OUT_RIGHT_MID, 0);

  label_bat = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_bat, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(label_bat, "Bat");
  lv_obj_align(label_bat, label_step, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

  batValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(batValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text(batValue, "?");
  lv_obj_align_y(batValue, label_bat, LV_ALIGN_OUT_RIGHT_MID, 0);
  lv_obj_align_x(batValue, lv_scr_act(), LV_ALIGN_CENTER, 5);

  prompt_notif = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(prompt_notif, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(prompt_notif, "You have mail");
  lv_obj_align(prompt_notif, label_bat, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  //lv_obj_set_hidden(prompt_notif, false);

  heartRateController.Start();
}

WatchFaceTerminal::~WatchFaceTerminal() {
  lv_obj_clean(lv_scr_act());
}

bool WatchFaceTerminal::Refresh() {
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    auto isCharging = batteryController.IsCharging() || batteryController.IsPowerPresent();
    if (isCharging) {
      lv_label_set_text_fmt(batValue, "%dC", batteryPercent);
    } else {
      lv_label_set_text_fmt(batValue, "%d", batteryPercent);
    }
    lv_obj_align_x(batValue, lv_scr_act(), LV_ALIGN_CENTER, 5);
  }

  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    if (bleState.Get() == true) {
    } else {
    }
  }

  notificationState = notificatioManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    //if (notificationState.Get() == true)
    //  lv_obj_set_hidden(prompt_notif, false);
    //else
      //lv_obj_set_hidden(prompt_notif, true);
  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if (currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();

    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime - dp);
    auto yearMonthDay = date::year_month_day(dp);

    auto year = (int) yearMonthDay.year();
    auto month = static_cast<Pinetime::Controllers::DateTime::Months>((unsigned) yearMonthDay.month());
    auto day = (unsigned) yearMonthDay.day();
    auto dayOfWeek = static_cast<Pinetime::Controllers::DateTime::Days>(date::weekday(yearMonthDay).iso_encoding());

    int hour = time.hours().count();
    auto minute = time.minutes().count();

    char minutesChar[3];
    sprintf(minutesChar, "%02d", static_cast<int>(minute));

    char hoursChar[3];
    sprintf(hoursChar, "%02d", hour);

    if (hoursChar[0] != displayedChar[0] || hoursChar[1] != displayedChar[1] || minutesChar[0] != displayedChar[2] ||
        minutesChar[1] != displayedChar[3]) {
      displayedChar[0] = hoursChar[0];
      displayedChar[1] = hoursChar[1];
      displayedChar[2] = minutesChar[0];
      displayedChar[3] = minutesChar[1];

      char timeStr[6];

      sprintf(timeStr, "%c%c:%c%c", hoursChar[0], hoursChar[1], minutesChar[0], minutesChar[1]);
      lv_label_set_text(label_time, timeStr);
      lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 25);
    }

    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      char dateStr[22];
      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) {
        sprintf(dateStr, "%d %s %d", day, dateTimeController.MonthShortToString(), year);
      } else {
        sprintf(dateStr, "%s %d %d", dateTimeController.MonthShortToString(), day, year);
      }
      lv_label_set_text(label_date, dateStr);
      lv_obj_align_origo(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get())
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    else
      lv_label_set_text_static(heartbeatValue, "---");

    lv_obj_align_x(heartbeatValue, lv_scr_act(), LV_ALIGN_CENTER, 5);
    lv_obj_align_y(heartbeatValue, heartbeatBpm, LV_ALIGN_IN_TOP_MID, 0);
  }

  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_align_x(stepValue, lv_scr_act(), LV_ALIGN_CENTER, 5);
    lv_obj_align_y(stepValue, label_step, LV_ALIGN_IN_TOP_MID, 0);
  }

  return running;
}
