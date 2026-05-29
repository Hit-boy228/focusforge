# Telegram Dialog Flows

## /start — Onboarding

```
User: /start
Bot:  👋 Привет, Alice! [MainMenu keyboard]
```

## /task — Create Task (multi-step)

```
User: /task
Bot:  📝 Введи название задачи:
User: Buy groceries
Bot:  🎯 Выбери приоритет: [🔵Low 🟡Medium 🟠High 🔴Critical]
User: [tap Medium]
Bot:  📅 Укажи дедлайн (или пропусти):
User: tomorrow
Bot:  🏷 Добавь теги (или пропусти):
User: #home
Bot:  ✅ Задача создана! [task card with actions]
```

## Quick Task Input

```
User: /task Buy milk tomorrow 18:00 p2 #home
Bot:  ✅ Задача создана! (one step, no prompts)
```

## /focus — Focus Session

```
User: /focus
Bot:  🎯 Выбери режим: [🍅Pomodoro 🧠DeepWork ⚙️Custom]
User: [tap Pomodoro]
Bot:  🎯 Сессия запущена! 25м [⏸Pause 🛑Stop]
...
Bot:  ✅ Сессия завершена! Что сделал?
User: Finished task review
Bot:  💾 Рефлексия сохранена. Отличная работа!
```

## Anti-Accidental Stop

```
User: /stop
Bot:  ⚠️ Остановить активную сессию? [✅Подтвердить ❌Отмена]
User: [tap Confirm]
Bot:  🛑 Сессия остановлена.
```

## /remind — Create Reminder

```
User: /remind
Bot:  🔔 Введи текст напоминания:
User: Call dentist
Bot:  🕐 Когда напомнить?
User: 15:30
Bot:  ✅ Напоминание установлено на сегодня 15:30
```

## Snooze Reminder

```
Bot:  ⏰ Напоминание: Call dentist [10м 30м 1ч Завтра ❌]
User: [tap 1ч]
Bot:  ✅ Отложено на 1 час
```

## /review — Weekly Review

```
User: /review
Bot:  [Weekly report card]
Bot:  💪 Что получилось хорошо на этой неделе?
User: Completed 3 big tasks, ran every day
Bot:  🤔 Что улучшить?
User: Better time blocking in the afternoon
Bot:  🎯 Главная цель на следующую неделю?
User: Launch feature X
Bot:  ✅ Ретроспектива сохранена! Хорошей недели! 🚀
```
