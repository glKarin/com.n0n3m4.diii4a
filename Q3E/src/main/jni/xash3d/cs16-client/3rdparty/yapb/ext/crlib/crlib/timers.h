//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

CR_NAMESPACE_BEGIN

// set the storage time function
class TimerStorage final : public Singleton <TimerStorage> {
private:
   float *timefn_ {};

public:
   explicit TimerStorage () = default;
   ~TimerStorage () = default;

public:
   void setTimeAddress (float *timefn) {
      timefn_ = timefn;
   }

public:
   float value () const {
      return *timefn_;
   }
};

// expose global timer storage
CR_EXPOSE_GLOBAL_SINGLETON (TimerStorage, timerStorage);

// invalid timer value
namespace detail {
   constexpr float kInvalidTimerValue = -1.0f;
   constexpr float kMaxTimerValue = 1.0e9f;
}

// simple class for counting down a short interval of time
class CountdownTimer {
private:
   float duration_ { 0.0f };
   float timestamp_ { detail::kInvalidTimerValue };

public:
   CountdownTimer () = default;
   explicit CountdownTimer (const float duration) { start (duration); }

public:
   void reset () { timestamp_ = timerStorage.value () + duration_; }

   void start (const float duration) {
      duration_ = duration;
      reset ();
   }

   void invalidate () {
      timestamp_ = detail::kInvalidTimerValue;
   }

public:
   bool started () const {
      return timestamp_ > 0.0f;
   }

   bool elapsed () const {
      return timestamp_ < timerStorage.value ();
   }

   float elapsedTime () const {
      return timerStorage.value () - timestamp_ + duration_;
   }

   float timestamp () const {
      return timestamp_;
   }

   float remainingTime () const {
      return timestamp_ - timerStorage.value ();
   }

   float countdownDuration () const {
      return started () ? duration_ : 0.0f;
   }
};

// simple class for tracking intervals of time
class IntervalTimer {
private:
   float timestamp_ { detail::kInvalidTimerValue };

public:
   IntervalTimer () = default;

public:
   void reset () {
      timestamp_ = timerStorage.value ();
   }

   void start () {
      timestamp_ = timerStorage.value ();
   }

   void invalidate () {
      timestamp_ = -detail::kInvalidTimerValue;
   }

public:
   bool started () const {
      return timestamp_ > 0.0f;
   }

   float elapsedTime () const {
      return started () ? timerStorage.value () - timestamp_ : detail::kMaxTimerValue;
   }

   bool lessThen (const float duration) const {
      return timerStorage.value () - timestamp_ < duration;
   }

   bool greaterThen (const float duration) const {
      return timerStorage.value () - timestamp_ > duration;
   }
};

CR_NAMESPACE_END
