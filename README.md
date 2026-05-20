# SimpleDB MVP

Паттерн: Facade. Класс `simpledb::Database` скрывает хранение, чтение файлов, парсинг и выполнение запросов за одним методом `execute()`.

## Сборка
```bash
./build.sh
```

## Запуск
```bash
./run.sh 127.0.0.1 8080
```

## Пример
```sql
CREATE DATABASE test;
USE test;
CREATE TABLE users (id INT, name TEXT, age INT);
INSERT INTO users VALUES (1, 'Vasya', 16), (2, 'Yulia', 67);
SELECT * FROM users;
SELECT name, age FROM users WHERE id = 1;
UPDATE users SET age = 17 WHERE name = 'Vasya';
DELETE FROM users WHERE age > 60;
DROP TABLE users;
DROP DATABASE test;
```
