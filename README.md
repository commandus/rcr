# База данных микросхем

## Термины

Компонентом называется радиодеталь одного из типов: резистор, конденсатор, индуктивность, микросхема.

Пассивные компоненты, как правило, имеют одну основную числовую характеристику - номинал.

Для пассивных компонент для поиска часто бывает достаточно указать только номинал.

Для микросхем, транзисторов, реле поиск обычно осуществляется по названию. 

Название транзистора, например, может выглядеть так: "2SD1825"

Типы компонентов на принципиальных схемах обозначаются символом - одной большой буквой латинского алфавита.

Например, тип резистор имеет символ R, емкость- C, микросхема- D.

Компоненты хранятся в пронумерованных коробках или кассах. Номер коробки - это десятичное число от 1 до 32768.

Коробки хранятся в шкафах, шкафы находятся в комнатах.

Одного номера, чтобы найти коробку, недостаточно. Для указания места хранения используется путь.

Путь - это серия номеров, разделенных знаком "-" (минус).

Например, в комнате 119 находится шкаф или большая коробка с номером 42, а в ней - маленькая коробка из под конфет с номером 13.

Тогда путь будет:

```
119-42-13
```

Первый номер - это комната, второй номер - шкаф или большая коробка, третий- место хранения компонента.

Когда нужно найти компонент, можно искать его во всех местах, или ограничить поиск комнатой. Тогда нужно будет указать путь 119.

Если надо ограничить поиск шкафом, нужно указать путь 119-42.

Если ограничения поиска нет, можно указать символ "*".

Карточкой называется запись с названием компонента (и номинала, если есть), ее типом и списком мест хранения.

Список мест хранения состоит из записей. Каждая запись включает в себя путь к коробке и количество в ней этого
компонента.

Из карточки можно узнать, где (в каких коробках), и сколько компонент есть в остатке в каждой коробке.

Для одного компонента может быть заведено несколько карточек учета, отличающихся особенностями исполнения (или по местам хранения, как удобно).

Карточка учета имеет уникальные

- номер (он используется внутри программы)
- имя (название)

и содержит 

- Компонент (например, микросхема D1218)
- Список мест хранения (номера коробок- пути)
- Опционально список особенностей исполнения микросхемы, например, тип корпуса
- nominal Числовое значение номинала для пассивных элементов

На карточку можно ссылаться по ее номеру или уникальному имени.

Для поиска карточек можно использовать или имя компонента, или номинал с единицей измерения для пассивных компонентов,
например:

- 100 кОм
- 20мкФ
- 2SD1825
- К155ЛА7 К-dip
- 
В имени компонента для поиска можно использовать подстановочные символы:

- * или % - последовательность любых символов
- _ - один символ

Например, при поиске '2S*'или '2S_1*' найдутся транзисторы '2SD1825' и '2SD1069'. 

Если указывается номинал, то за числом номинала нужно указать единицу измерения с приставкой порядка величины.

Номинал - это целое положительное число.

Компоненты с номиналами:

|Символ| Тип пассивного компонента  | Единица измерения | Допустимые приставки          |
|------|----------------------------|-------------------|-------------------------------|
| R    | Резистор                   | Ом                | к, М, Г, Т, П, Е, З, И, Р, Кв |
| С    | Кондесатор                 | Ф                 | мк, н, п, ф, а, з, и, р, кв   |
| L    | Индуктивность              | Гн                | мк, н, п, ф, а, з, и, р, кв   |

С приставками номинал обычно округляется до трех цифр, для поиска этого достаточно.

Все типы компонент:

|Символ| Тип компонента                |
|------|-------------------------------|
|   A  | Устройства                    |
|   B  | Микрофоны, громкоговорители   |
|   C  | Конденсаторы                  |
|   D  | Интегральные схемы            |
|   E  | Разные элементы               |
|   F  | Плавкие предохранители        |
|   G  | Источники питания             |
|   H  | Индикаторы                    |
|   I  | Крепления (*)                 |
|   J  | Провода (*)                   |
|   K  | Реле                          |
|   L  | Дроссели                      |
|   M  | Двигатели                     |
|   N  | На выброс (**)                |
|   O  | Проекты (**)                  |
|   P  | Счетчики                      |
|   Q  | Выключатели                   |
|   R  | Резисторы                     |
|   S  | Переключатели                 |
|   T  | Трансформаторы                |
|   U  | Выпрямители                   |
|   V  | Диоды, тиристоры, транзисторы |
|   W  | Антенны                       |
|   X  | Гнезда                        |
|   Y  | Электромагнитный привод       |
|   Z  | Кварцевые фильтры             |

Символы I, J, N, O имеют особое значение, в таблице отмечены звездочками:

(*) -  на схемах буквами не обозначается, символ выбран условно для проводов и креплений

(**) - зарезервировано для перевода компонент в расход в особых случаях

### Место хранения (коробка, пакет) 

Место хранения имеет путь в виде серии номеров, разделяемых символом-разделителем,
номера означают:

- номер комнаты
- номер шкафа
- номер кассы
- номер коробки в кассе

Символ-разделитель- знак минус ("-").

Например, 219-1 это железный шкаф в комнате 219, 119-23- 23-я коробка в кабинете 119.

Максимальный размер массива- 4, номер - положительное число от 1 до 32767 (один бит не используется).

Замечание: Sqlite3 имеет тип Int64, но нет типа Uint64, поэтому в первом числе е пути можно записать 32767, 
но не 65535 

В коробке лежат несколько компонент разных видов.

Комната, шкаф, пакет(ящик), коробка имеют номера.

Место хранения указывается в списке в карточке учета, она указывает, сколько компонент одного типа лежит в коробке.

Место хранения компонента содержит в своей записи

- Карточка (ее номер)
- номер коробки
- количество в коробке

Общее количество компонент любого типа может быть подсчитано суммированием количеств в каждом месте хранения каждого компонента.

### Операции

Операции - это изменение в карточке количества в местах хранения следующих типов

- Пополнение + <число>. Добавляет в коробку заджанное число компонент
- Выдача - <число>. Уменьшает в коробке количество компонент на заданное число
- Присвоение = <число>. Устанавливает количество в коробке компонент
- Перемещение / <путь> в другую коробку, заданную путем

Примеры:

Добавить в коробку 119-42-13 два транзистора
```
2SD1825 119-42-13 +2
```

Вынуть из коробки 119-42-13 два транзистора
```
2SD1825 119-42-13 -2
```

Вынуть из всех коробок в комнате 119 по два транзистора из каждой коробки, где она есть:
```
2SD1825 119 -2
```
Если в коробках нет таких транзисторов, ничего не произойдет.

Если в каких-то коробках есть одна микросхема, вынется одна.

Добавить в коробки в комнате 119 по два транзистора в те коробки, где они уже были:
```
2SD1825 119 +2
```
Если в коробках не было таких транзисторов, ничего не произойдет.


Переместить из коробки 119-42-13 два транзистора в коробку 119-42-14
```
2SD1825 119-42-13 /2 119-42-14
```

Переместить из коробки 119-42-13 два транзистора 2SD1825 в коробку 119-42-14
```
2SD1825 119-42-13 / 119-42-14
```

Переместить из коробки 119-42-13 все в коробку 119-42-14
```
* 119-42-13 /2 119-42-14
```

Агрегация

- count - подсчитывеает количество карточек
- sum - подсчитывеает количсество компонентов

Изменения карточек, такие как 

- Создание карточки
- Удаление карточки

не затрагивает изменения количества в местах хранения и не записывается в журнал

### Журнал операций

В журнале отмечается время операции, кто внес изменение, а также сама карточка и величина изменения

## Программы и утилиты

- rcr-svc - сервис, gRPC порт 50051, HTTP/1.1 порт 8050
- rcr-cli - CLI клиент
- box преобразует идентификатор коробки в строку, и наоборот
- mk_db создает файл пустой базы данных Sqlite3

### rcr-cli

Файл конфигурации rcr.config ищется в следующем порядке:

- текущий каталог
- /etc
- $HOME
- каталог, где размещена программа rcr-cli 

В файле конфигурации rcr.config указаны имя пользователя и пароль, разделенный пробелом:

```
<user-name> <password>
```

По умолчанию имя пользователя: SYSDBA и пароль: masterkey

Например, 
```
./rcr-cli -u SYSDBA -p masterkey card "KT315 119-1 = 2"
```
устанавливает число транзисторов KT315 в коробке 119-2 равным двум.

Например, файл t.rcr содержит "команды"

```
symbol D
ExampleK1 119-1 = 1
ExampleK2 119-2 = 2
ExampleK3 119-3 = 3
quit
```

их можно выполнить в Windows так:
```
type t.rcr | rcr-cli -u SYSDBA -p masterkey
```
В Linux вместо type используйте cat.

### rcr-svc

Обслуживает GRPC клиентов на порту 50051.

Может обслуживать веб клиентов на порту 8050, запросы и ответы выдаются обьетками, сериализованными JSON.

Опции

- -v детализированный вывод в консоль
- -j включает встроенный веб сервис, 
- -l <номер порта> назначает порт gRPC сервиса (по умолчанию 50051)
- -P <номер порта> назначает порт HTTP/1.1 сервиса (по умолчанию 8050)
- -r <путь папки> назначает корневой каталог, где размещены файлы. По умолчанию- html 

#### JSON запросы встроенного веб сервиса

Встроенный веб сервис обслуживает часть запросов gRPC, а именно, следующие пути в HTTP POST запросах:

- /login
- /getDictionaries
- /getSettings
- /setSettings
- /chPropertyType
- /chCard
- /chBox
- /cardQuery
- /getBox
- /lsUser
- /chUser
- /chGroup
- /chGroupUser
- /importExcel

Соответствуют одноименным gRPC методам.

Параметры запроса передаются методом POST в JSON. Параметры перечислены в proto/rcr.proto 

##### Словари

```
wget -q -S -O - --post-data '{"locale_id":0,"flags":0}' http://localhost:8050/getDictionaries
{
 "operation": [
  {
   "id": "1",
   "symbol": "+",
   "description": ""
  },...
 ],
 "symbol": [
  {
   "id": "1",
   "sym": "A",
   "description": "Устройства",
   "unit": "",
   "pow10": 0
  },...
 ],
 "property_type": [
  {
   "id": "1",
   "key": "K",
   "description": "корпус"
  },...
 ]
}

```

##### Запрос

```
wget -q -S -O - --post-data '{"user":{"id":"4","token":"931400716","name":"andrey.ivanov@ikfia.ysn.ru","password":"x","rights":1},"query":"*",
"measure_symbol":"","list":{"offset":0,"size":20}}' http://localhost:8050/cardQuery
{
 "rslt": {
  "code": 0,
  "id": "0",
  "description": "",
  "count": "318",
  "sum": "0"
 },
 "cards": {
  "cards": [
   {
    "card": {
     "id": "97",
     "name": "130/270mA  DIP8 IR (MN)",
     "uname": "130/270MA  DIP8 IR (MN)",
     "symbol_id": "4",
     "nominal": "0"
    },
    "properties": [],
    "packages": [
     {
      "id": "97",
      "card_id": "97",
      "box": "33495526523666432",
      "qty": "5",
      "box_name": "119-1-2"
     }
    ]
   },
   ...
```

##### Загрузка файла Excel
```
wget -q -S -O - --post-data '{"user":{"id":"4","token":"931400716","name":"SYSDBA","password":"x","rights":1},"symbol":"C","prefix_box":"62205969853054976","number_in_filename":true,"operation":"+","file":[{"name":"27 Конденсаторы  электролиты_Золотовский.xlsx","content":"UEsDBBQABgAIAAAA...' http://localhost:8050/importExcel
```

##### Журнан карточки 1010
```
wget -q -S -O - --post-data '{"user":{"id":"2","token":"956247633","name":"SYSDBA","password":"x","rights":1},
"list":{"offset":0,"size":20},"box_id":0,"card_id":1010}' http://localhost:8050/lsJournal
```

### Загрузка перечня из файлов книги Excel

Коробки имеют номера.

Коробки вкладываются друг в друга.

Файл книги Excel в названии может иметь несколько чисел с указанием коробки.

Книга Excel может содержать несколько таблиц с произвольными именами.

Имена таблиц в создании иерархии коробок не участвуют.

Колонка "A" файла Excel должна содержать номер коробки.

### Проверка файлов

Проверка

```
xlsx-list "~/src/rcr/data" -b 121
```

Параметр -b хадает номер первой коробки (он может совпадать с номер комнаты, где искать коробку)

#### Загрузка файлов

Например, добавим микросхемы из файлов Excel в каталоге ../data: 

```
./rcr-cli xlsx-add-d ../data -b 219
```

Последняя буква в xlsx-add-X указывает тип компонента:

- xlsx-add-A Устройства
- xlsx-add-B Микрофоны, громкоговорители
- xlsx-add-C Конденсаторы
- xlsx-add-D Интегральные схемы
- xlsx-add-E Разные элементы
- xlsx-add-F Плавкие предохранители
- xlsx-add-G Источники питания
- xlsx-add-H Индикаторы
- xlsx-add-K Реле
- xlsx-add-L Дроссели
- xlsx-add-M Двигатели
- xlsx-add-P Счетчики
- xlsx-add-Q Выключатели
- xlsx-add-R Резисторы
- xlsx-add-S Переключатели
- xlsx-add-T Трансформаторы
- xlsx-add-U Выпрямители
- xlsx-add-V Диоды, тиристоры, транзисторы
- xlsx-add-W Антенны
- xlsx-add-X Гнезда
- xlsx-add-Y Электромагнитный привод
- xlsx-add-Z Кварцевые фильтры 

- В каталоге ../data находятся файлы с именами:

```
Мксх_Золотовский_10.xlsx
Мксх_Золотовский_11.xlsx
```

В колонке "A" в каждой электронной таблице указывается номер последней в иерархии в вложенности коробки.

Тогда коробки будут иметь составные номера:

```
121-10-1 - первая коробочка из коробки 10 в кабинете 121 ()
121-10-2 - вторая коробочка из коробки 10 в кабинете 121
...
121-11-1 - первая коробочка из коробки 11 в кабинете 121
121-11-2 - вторая коробочка из коробки 11 в кабинете 121
...
```

### В интеркативном режиме

```
./rcr-cli -d localhost
```

Просмотр статистики в файлах(подсчитывает количество компоент) без долбавления в базу данных

```
import ../data
```

Просмотр как будут добавлены компоненты из файла с префиксом коробки 119 и компонентогми по умолчанию- резистор
без добавления в базу данных

```
sheet ../data R 119
```

#### Добавление 

С префиксом коробки 119 и чтением номера коробки в имени файла (если есть)

```
import ../data R 119
```

С префиксом коробки 119 и запретом чтения номера коробки в имени файла

```
import ../data R 119 no-num
```

## Свойства

Компонент может иметь свойства.

Свойство имеет короткое название, и полное название (описание).

Короткое название обычно состоит из одной или двух букв. Регистр букв различается.

### Список свойств

В ../rcr-cli введите
```
property list
```
и получите список свойств, которыми можно охарактеризовать компоненты, например:
```
K	корпус
A	точность
```

### Использование свойств

В ../rcr-cli свойства указываются после названия компонента или его номинала, например, в поиске:
```
100 кОм A:10%
100 кОм A:5% P:1Вт
```

или присвоении числа штук хранения:

```
100 кОм A:10% 119-2 = 7
100 кОм A:5% P:1Вт 119-2 = 6
```

и в других операциях.

Карточки с разными свойствами учитываюься отдельно, то есть, например,
карточка "100 кОм A:10% 119-2" не входит в карточку "100 кОм 119-2".

### Добавление свойства

```
property + A точность
```
 
100 кОм
100 кОм 119-1 = 5
100 кОм :5% K:DIP 119-2 = 6
100 кОм :10% 119-2 = 7


100 кОм
100 кОм 119-1 = 5
100 кОм :5% K:DIP 119-2 = 6
100 кОм :10% 119-2 = 7

### Удаление свойства

```
property - A
```

100 кОм
100 кОм 119-1 = 5
100 кОм A:5% K:DIP 119-2 = 6
100 кОм A:10% 119-2 = 7

### Редактирование свойств

В ../rcr-cli можно изменить полное имя (описание) свойства:
```
property = A точность
```
Короткое название изменить нельзя (можно сделать в десктоп клиенте).

## Операции и журнал операций

Сотрудники добавляют и забирают детали, при этом в месте хранения записывается остаток деталей.

Остаток не может быть отрицательным.

Действия сотрудников отмечаются в журнале.

Получение остатка в местах хранения по названию (номиналу) по компонентам

## Замечания разработчикам

Удаленный вызов cardQuery() производит большинство манипуляция с карточками.

- user имя пользователя, производящего манипуляцию
- query текст запроса
- measure_symbol тип компонентов, ограничивающего действие запроса
- list смещение (начальный индекс в списке) и максимальное число в списке, имеет значение, если запрос возвращает наблр карточек

В query отправляются такие же запросы, как ./rcr-cli

Результат возвращается в OperationResponse в code (0- операция успешно завершена, не 0- код ошибки) и description- 
краткое описание ошибки.

Если в запросе задан агрегат count или sum, результат возвращается
в OperationResponse в count и sum.

### Список мест хранения

getBox()

Места хранения обновляются при манипуляциях с карточками.

После вызова cardQuery() список мест хранения может измениться.


### Детали реализации

Первоначальная диаграмма объектов находится в файле diagram.uxf. Для просмотра и редактирования нужно установить плагин
Visual Code UMLet.

Для разделения прав между группами пользователей в веб приложении выбираются разные серввисы по номерам портов, хотя в
приложении есть понятие групп, проверка полномочий в операциях над принадлежащими и не принадлежащими группам коробками
не реализована.

Пример списка компонент:

| Symbol | Name        | Unit | Pow10 |
|--------|-------------|------|-------|
| R      | Резистор    | Ом   | 0     |
| С      | Конденсатор | Ф    | -9    |
| V      | 2SD1824     |      |       |
| V      | 2SD1825     |      |       |

степень 10 для номинала, записываемой в базе данных. Например, 0- это 10^0 = 1, в примере номинал хранится как целое число в Омах. Для конденсаторов -9 - это 10^-9 = 1/1000000000

В поле Unit для микросхем можно указывать, например, условное обозначение назначения (телевизионная, микроконтроллер,..)
User
Для микросхем в карточке может указываться тип корпуса в списке особенностей и (или) в поле числовое значение - число ножек или другой признак, по которому можно различить тип корпуса.

В поиске вводятся: 

- Имя компонента
- Номинал (если не пустое), используя частицы для указания порядка степени (кило, мега, милли, микро, пико)
- Через знак "-" опциональные особенности, например, тип корпуса

Для пассивных элементов в карточке в поле числовое значение указывается номинал (для резисторов сопротивление в Омах, для конденсаторов емкость в пикофарадах)

Номинал отображается в виде трех цифр, слитно с ними частица и единица измерения.

Символ компонента (R, C) с учетом того, что указывается единица измерения, не нужен.

Общее количество подсчитывается суммированием количеств в каждом месте хранения

#### Таблицы

Таблицы базы данных:

- Box Коробка
- BoxGroup Принадлежность коробок группам пользователей
- Card Карточка учета номенклатуры - микросхема с учетом исполнения(корпуса) и запасы в разных местах хранения                    
- Group Группа пользователей
- Journal Журнал операций
- Package Место хранения. Место хранения может хранить несколько типов компонент
- Property Дополнительные свойства - описание особенностей исполнения, например, тип корпуса, или номинальное напряжение.
- ServiceSettings настройки для интеерфейса пользователя, связанный с сервисом, например, последняя строка запроса

Словари:

- Symbol Условное обозначение компонента (R, C, ...) и единица измерения номинала (для пассивных элементов)
- Operation '+'- добавление,  '-'- уменьшение, '='- установка значения '/'- разделение (перемещение части компонент из одной коробки в другую) 
- PropertyType Типы свойств ('A'- тип корпуса, 'V'- номинальное напряжение, 'A'- максимальный ток, 'P'- рассеиваемая мощность, '%'- точность) 

Вспомогательные таблицы:

- User Пользователи и их права
- GroupUser Пользователи в группе

Схема базы данных сгененирована скриптом, вызываемым в момент сборки приложения по .proto файлу.

При работе с базой данных нужно учитывать, что логика реализована в приложении сервиса. Например,
при прямой работе с базой данных при добавлении карточки, нужно проверить наличие коробки, если ее нет,
создать, в журнале отметить эти события.

##### Box

CREATE TABLE IF NOT EXISTS "Box" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "box_id" INTEGER NOT NULL,
  "name" TEXT NOT NULL,
  "uname" TEXT NOT NULL);
CREATE INDEX "Box_box_id_i"
  ON "Box" ("box_id");

box_id типа Int64 приложение интепретирует как четыре двухбайтных слова, страшее слово- знаковое, остальные- баззнаковые.

Для перевода box_id в читаемую последовательность вида 119-1-2 и наоборот используйте утилиту box.

##### BoxGroup

CREATE TABLE IF NOT EXISTS "BoxGroup" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "group_id" INTEGER NOT NULL,
  "box_id" INTEGER NOT NULL);

##### Card

CREATE TABLE IF NOT EXISTS "Card" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "name" TEXT NOT NULL,
  "uname" TEXT NOT NULL,
  "symbol_id" INTEGER NOT NULL,
  "nominal" INTEGER NOT NULL);

##### Group

CREATE TABLE IF NOT EXISTS "Group" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "name" TEXT NOT NULL);

##### GroupUser

CREATE TABLE IF NOT EXISTS "GroupUser" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "group_id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL);

##### Journal

CREATE TABLE IF NOT EXISTS "Journal" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "dt" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "package_id" INTEGER NOT NULL,
  "operation_symbol" TEXT NOT NULL,
  "value" INTEGER NOT NULL);

##### Operation

CREATE TABLE IF NOT EXISTS "Operation" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "symbol" TEXT NOT NULL,
  "description" TEXT NOT NULL);

##### Package

CREATE TABLE IF NOT EXISTS "Package" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "card_id" INTEGER NOT NULL,
  "box" INTEGER NOT NULL,
  "qty" INTEGER NOT NULL);
CREATE INDEX "Package_card_id_i"
  ON "Package" ("card_id");
CREATE INDEX "Package_box_i"
  ON "Package" ("box");

##### Property

CREATE TABLE IF NOT EXISTS "Property" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "card_id" INTEGER NOT NULL,
  "property_type_id" INTEGER NOT NULL,
  "value" TEXT NOT NULL);

##### PropertyType

CREATE TABLE IF NOT EXISTS "PropertyType" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "key" TEXT NOT NULL,
  "description" TEXT NOT NULL);
CREATE UNIQUE INDEX "PropertyType_key_i"
  ON "PropertyType" ("key");

##### ServiceSettings

CREATE TABLE IF NOT EXISTS "ServiceSettings" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "name" TEXT NOT NULL,
  "addr" TEXT NOT NULL,
  "port" INTEGER NOT NULL,
  "last_component_symbol" TEXT NOT NULL,
  "last_box" INTEGER NOT NULL,
  "last_query" TEXT NOT NULL,
  number_in_filename INTEGER, last_excel_file TEXT, last_excel_dir TEXT, last_import_box INTEGER, show_dialog_on_import_finish INTEGER
);

##### Symbol

CREATE TABLE IF NOT EXISTS "Symbol" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "sym" TEXT NOT NULL,
  "description" TEXT NOT NULL,
  "unit" TEXT NOT NULL,
  "pow10" INTEGER NOT NULL);

##### User

CREATE TABLE IF NOT EXISTS "User" (
  "id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "name" TEXT NOT NULL,
  "password" TEXT NOT NULL,
  "token" INTEGER NOT NULL,
  "rights" INTEGER NOT NULL);

#### Особенности исполнения Property

Словарь Property содержит дополнительные свойства, которые могут быть отмечены в карточке:

- тип корпуса микросхемы
- допустимое отклонение от номинала (%)
- ...

Словарь может быть произвольно дополнен другими свойствами.

Свойство имеет ключ (typ), используемый для идентификации свойства, и значение.

| typ | value                  |
|-----|------------------------|
| K   | корпус                 |
| V   | номинальное напряжение |
| А   | предельный ток         |
| P   | рассеиваемая мощность  |
| %   | точность в %           |

### Формальное описание строки запроса в cardQuery

```

---<описание компонента>---+-----------------+------------
                           |                 
                          _+---<коробка>---+-----------------------------+---------------------+--
                                           |                                                   |
                                           +--<операция>--<количество>---+-----------------+---+
                                                                         |                 |
                                                                         +---<операнд 2>---+
(описание компонента)---+---<наименование>--------------------------------------+--
                        |                                                       |
                        +--<наименование с подстановочными знаками>-------------+
                        |                                                       |
                        +--<число номинала>--+---------------+---<ед.измерения>-+
                                             |               |
                                             +--<приставка>--+
(коробка)---+---<число>---+---------
            |             |
            <----"-"------<

(операция)--+---"+"----+---------
            |          |
            +----"-"---+
            |          |
            +----"="---+
            |          |
            +----"/"---+

(приставка)--+---"мк"--+--
             |         |
             +---"п"---+
             |
             ...
```

операнд 2 применятся только в операции "/".

### Инструменты

- CMake
- Visual Studio 2022 17.4.5 (Visual C++)
- vcpkg 2021-12-09 (Windows)
- Gettext
- компилятор odb (2.4.0)
- компилятор protoc (libprotoc 3.18.0)

### Linux

Для Linux для glade установите утилиту xmllint для удаления пробелов в XML файле ресурса.

```
sudo apt install libxml2-utils
```

### Windows

CMake должен быть установлен с включением пути выполнимых файлов CMake в переменную окружения PATH (по умолчанию нет).

Для Windows скачайте https://www.codesynthesis.com/download/odb/2.4/odb-2.4.0-i686-windows.zip

Распакуйте odb-2.4.0-i686-windows.zip в корень, нужны папки

- C:\bin
- C:\etc
- C:\mingw

Папку C:\bin включить в переменную окружения PATH.

Для Windows скопируйте 

- protoc.exe
- grpc_cpp_plugin.exe
- C:\git\vcpkg\installed\x64-windows-static\tools\protobuf\*.dll
- C:\git\vcpkg\buildtrees\grpc\x64-windows-rel\*.dll
- из папки в vcpkg утилиты gettext в папку, которая указана в переменной окружения PATH

в указываемый переменной окружения PATH путь, например, C:\bin

В скрипте tools/generate-code.ps1 в строке 2 исправьте путь к плагину
```
$GRPC_PLUGIN = "c:\p\bin\grpc_cpp_plugin.exe"
```
Плагин указывается по полному пути, в переменной PATH плагин grpc компилятора protoc
не ищет. В linux скрипте используется `which` для подстановки полного пути .

### Установка vcpkg в Windows:

```
cd \git
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

### Зависимости

- ICU / Intl
- Xlnt
- Protobuf
- gRPC
- unofficial-sqlite3/SQLite3(Windows)
- or unofficial-postgresql/PostgreSQL(Windows)
- Xlnt - есть в vcpkg для Windows, но нет в пакетах Ubuntu
- libmicrohttpd (если включена опция ENABLE_HTTP)
- libldap

Установка в Linux зависимостей из пакетов:

```
sudo apt install grpc-proto libgrpc++-dev libgrpc-dev protobuf-compiler-grpc protobuf-compiler libprotobuf-dev \
libc-ares-dev odb libodb-sqlite-2.4 libicu-dev libmicrohttpd-dev gettext sqlite3 libsqlite3-dev libldap-dev
```

[Install ODB unix](https://codesynthesis.com/products/odb/doc/install-unix.xhtml)

```
sudo apt install odb libodb-pgsql-2.4
```

Проверьте версию компилятора odb

```
odb --version
ODB object-relational mapping (ORM) compiler for C++ 2.4.0
```

Если выдает ошибку:

```
g++-10: error: No such file or directory
```

или

```
g++-12: error: No such file or directory
```

Установите запрашиваемый компилятор g++-10:

```
sudo apt install g++-10
```
или
```
sudo apt install g++-12
```

### Xlnt

В Linux Xlnt нужно собрать вручную:

```
git clone https://github.com/tfussell/xlnt.git
cd xlnt
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make
sudo make install
ls /usr/local/lib/libxlnt.so.1.5.0
ls /usr/local/include/xlnt/xlnt.hpp
```

### libicu

Посдедний релиз C++11 не C++17 [ICU 73](https://github.com/unicode-org/icu/archive/refs/tags/release-73-2.tar.gz)

Если в репозитории нет версии 61 и выше, собрать из исходников:
```
git clone https://github.com/unicode-org/icu.git
cd icu/icu4c/source
./configure
make
sudo make install
```

### Установка зависимостей в Windows

```
cd \git\vcpkg
vcpkg install gettext[tools]:x64-windows libodb:x64-windows libodb-sqlite:x64-windows protobuf:x64-windows grpc:x64-windows 
icu:x64-windows xlnt:x64-windows libmicrohttpd:x64-windows
```

Static 
```
cd \git\vcpkg
vcpkg install libodb:x64-windows-static libodb-pgsql:x64-windows-static libodb-sqlite:x64-windows-static 
sqlite3:x64-windows-static grpc:x64-windows-static protobuf:x64-windows-static libmicrohttpd:x64-windows-static
```

### Установка odb

```
cd \git\vcpkg
vcpkg install libodb:x64-windows libodb-pgsql:x64-windows libodb-sqlite:x64-windows protobuf:x64-windows grpc:x64-windows
cd \src\rcr
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
```

```
vcpkg install libodb:x64-windows-static
```

https://www.codesynthesis.com/download/odb/2.4/libodb-2.4.0.zip

PostgreSQL backend

- libodb-pgsql (with libpq)

Install:

```
vcpkg install libodb-pgsql:x64-windows-static
```

SQLite backend

- libodb-sqlite

```
vcpkg install libodb-sqlite:x64-windows-static
```

or download from https://www.codesynthesis.com/download/odb/2.4/libodb-pgsql-2.4.0.zip
upgrade solution from 10 to 14 and build release


### Альтернативные способы установки зависимостей 

[Build and install gRPC and Protocol Buffers instructions](https://grpc.io/docs/languages/cpp/quickstart/)

tools/install-grpc.sh script installs libgrpc++.so.1.24.3

libprotoc 3.12.4
```
sudo apt install --reinstall grpc-proto libgrpc++-dev libgrpc-dev protobuf-compiler-grpc protobuf-compiler libprotobuf-dev
```

```
git clone --recurse-submodules -b v1.24.3 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_SSL_PROVIDER:STRING=package ../..
make -j 1
sudo make install
popd
```

gRPC depends on c-ares
```
sudo apt install libc-ares-dev
```

### Создание решения в Windows

Для создания решения Visual Studio воспользуйтесь CMake:

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
или
cmake -A x64 -D CMAKE_TOOLCHAIN_FILE=c:/git/vcpkg/scripts/buildsystems/vcpkg.cmake -D VCPKG_TARGET_TRIPLET=x64-windows ..
```

Проверьте proto/rcr.proto file.

Determine which classes are persistent in the odb/pragmas.pgsql.hxx.

List classes and members persistent in the relational database in the odb/pragmas.pgsql.hxx file
e.g.

```
	ODB_TABLE(Card)
		ODB_STRING(Card, name)
      ...
```

Then call ./tools/generate-code.ps1

./tools/generate-code.ps1 do:

- generate serialziation & protocol support classes
- generate object relation mapping (ORM) classes

### Serialzation & protocol support classes

Generate in the gen/ subdirectory protobuf serialization classes  (.pb.cc, .pb.h),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) using protoc compiler and 
protoc's grpc c++ plugin.

Object relation mapping (ORM)

Generate in the gen/ subdirectory ODB ORM mapping code(.pb-odb.cxx, .pb-odb.ixx, .pb-odb.hxx),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

by the proto/rcr.proto 

### Tools

- .\tools\generate-code.ps1 - Generate
- .\tools\clean-code.ps1 - Clean


#### Linux

Check grpc_cpp_plugin is installed
```
which grpc_cpp_plugin
```

If not, add grpc_cpp_plugin to the $PATH

```
cd ~/src-old/third_party/grpc/bins/opt
sudo cp grpc_cpp_plugin  grpc_node_plugin  grpc_objective_c_plugin  grpc_php_plugin  grpc_python_plugin  grpc_ruby_plugin /usr/local/bin
```


find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(main PRIVATE unofficial::sqlite3::sqlite3)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb-pgsql)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb-sqlite)

find_package(gRPC CONFIG REQUIRED)
target_link_libraries(main PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)

find_package(modules CONFIG REQUIRED)
target_link_libraries(main PRIVATE re2::re2 c-ares::cares)

find_package(protobuf CONFIG REQUIRED)
target_link_libraries(main PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)


C:\git\vcpkg\installed\x64-windows-static\tools\protobuf\protoc
C:\git\vcpkg\installed\x64-windows-static\lib\ C:\git\vcpkg\installed\x64-windows-static\lib\ 

#### Generate

Generate files:

- rcr.grpc.pb.cc
- rcr.grpc.pb.h
- rcr.grpc.pb.h
- rcr.pb-odb.cxx
- rcr.pb-odb.hxx
- rcr.pb-odb.ixx
- rcr.pb.cc
- rcr.pb.h
- rcr.pb.sql

In the PowerShell execute:
```
.\tools\generate-code.ps1
```

If errors occured see Bugs section.

#### Clean files

In the PowerShell execute:
```
.\tools\clean-code.ps1
```

#### CMake

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake
```

#### Bugs

##### Windows script generate-code.ps1 policy issue

Generating C:/git/rcr/gen/rcr.grpc.pb.cc, C:/git/rcr/gen/rcr.pb.cc, C:/git/rcr/gen/rcr.pb-odb.cxx, C:/git/rcr/gen/rcr.grpc.pb.h, C:/git/rcr/gen/rcr.pb.h, C:/git/rcr/gen/rcr.pb-odb.hxx
C:/git/rcr/tools/generate-code.ps1 : File C:\git\rcr\tools\generate-code.ps1 cannot be loaded. The file C:\git\rcr\tools\generate-code.ps1 is not digitally signed. You cannot run this script on the current system. For more information about running scripts and setting execution policy, see about_Execution_Policies at https:/go.microsoft.com/fwlink/?LinkID=135170.

PowerShell run as administrator:
```
Set-ExecutionPolicy -ExecutionPolicy Bypass
```
##### gcc

tools/generate-code.ps1 on odb calls gcc

odb exit with gcc error :

```
C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h:124:29: error: temporary of non-literal type 'google::protobuf::internal::CallOnceInitializedMutex<std::mutex>' in a constant expression
   constexpr WrappedMutex() {}
```

[StackOverflow explanation](https://stackoverflow.com/questions/69232278/c-protocol-buffer-temporary-of-non-literal-type-googleprotobufinternal)


Solution

Remove (comment) line 124 in the C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h
(C:\git\vcpkg\packages\protobuf_x64-windows\include\google\protobuf\stubs\mutex.h)

```
#if defined(__QNX__)
  constexpr WrappedMutex() = default;
#else
// constexpr <-- comment this!
  WrappedMutex() {}
#endif
```
#### Visual Studio C++17 deprecated issue

On error 'ptr_fun': is not a member of visual studio
see https://stackoverflow.com/questions/48882439/how-to-restore-auto-ptr-in-visual-studio-c17

1. Project > Properties > C/C++ > Preprocessor > Preprocessor Definitions and add _HAS_AUTO_PTR_ETC=1. Do so for all configurations and platforms.
2. If you use a precompiled header then you probably favor defining the macro there. Before any #includes, insert #define _HAS_AUTO_PTR_ETC 1.


#### odb

To see error description, change console code page:
```
chcp 1251
```

#### protoc

Windows issue:

```
gen/rcr.pb.h:10:40: fatal error: google/protobuf/port_def.inc: No such file or directory
#include <google/protobuf/port_def.inc>
```

Set valid path in tools/generate-code.ps1

```
$PROTOBUF_INC = "C:\git\vcpkg\packages\protobuf_x64-windows-static\include"
```

#### Python client

```
sudo python3 -m pip install grpcio
python3 -m pip install grpcio
python3 -m pip install grpcio-tools
```

##### Generate python code from the proto file

```
cd proto
python3 -m grpc_tools.protoc -I. --python_out=. --pyi_out=. --grpc_python_out=. rcr.proto
cp
``` 

##### Run an example

```
cd python
python3 rcr_client.py
cp
``` 


### Локализация

После правки po/rcr-cli.ru_RU.UTF-8.po запустите
```
tools/update-translation.sh
```
Созданный файл скопируйте в /usr/share/locale/ru/LC_MESSAGES/: 

```

msgfmt -o locale/ru/LC_MESSAGES/rcr-cli.mo po/rcr-cli.ru_RU.UTF-8.po
sudo cp locale/ru/LC_MESSAGES/rcr-cli.mo /usr/share/locale/ru/LC_MESSAGES/rcr-cli.mo
sudo cp locale/ru/LC_MESSAGES/rcr-cli.mo /usr/local/share/locale/ru/LC_MESSAGES/rcr-cli.mo
sudo cp locale/ru/LC_MESSAGES/*.mo /usr/share/locale/ru/LC_MESSAGES/
```

```
scp locale/ru/LC_MESSAGES/*.mo user@kb-srv.ysn.ru:~/rcr/locale/ru/LC_MESSAGES
ssh user@kb-srv.ysn.ru
sudo cp locale/ru/LC_MESSAGES/*.mo /usr/share/locale/ru/LC_MESSAGES/
```

Скрипт tools/l10n создает начальный файл. Файл уже существует в проекте, скрипт вызывать не надо.

### Сборка в Centos 8 в докере

- пакет libicu-devel версия 60, нужна 61 и не выше 73.
- grpc нет в репозитории
- нет заголовка /usr/include/xlocale.h

Создать докер

```
tools/mkdocker.sh
```

Запустить докер
```
docker run -itv /home/andrei/src:/home/andrei/src micro:centos bash

ln -s /usr/include/locale.h /usr/include/xlocale.h

# монтируем папку с исходниками
docker run -itv /home/andrei/src:/home/andrei/src micro:centos bash
# направить репозиторий на зеркало
cd /etc/yum.repos.d/
sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
sudo yum -y update
yum install git wget yum which cmake mc gcc clang sqlite-devel install libmicrohttpd gettext-devel


wget https://codesynthesis.com/download/odb/2.4/odb-2.4.0-1.x86_64.rpm
rpm -i odb-2.4.0-1.x86_64.rpm

wget https://codesynthesis.com/download/odb/2.4/libodb-2.4.0.tar.gz
tar xvf libodb-2.4.0.tar.gz
cd libodb-2.4.0
./configure 
make
make install
cd ..

wget https://codesynthesis.com/download/odb/2.4/libodb-sqlite-2.4.0.tar.gz
tar xvfz libodb-sqlite-2.4.0.tar.gz 
cd libodb-sqlite-2.4.0
./configure 
make
make install

# https://grpc.io/docs/languages/cpp/quickstart/
git clone --recurse-submodules -b v1.56.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF ../..
make -j 4
make install
popd

# Соберите Xlnt, как написано в разделе Xlnt
# Соберите libicu
```

#### закоммитить образ 

```
docker ps -a
docker commit stoic_ramanujan
docker images
docker tag c30cb68a6443 micro:centos
# удалить закрытье контейнеры
docker rm $(docker ps -qa --no-trunc --filter "status=exited")

```


#### Копируем файлы в Centos 8

```
ssh user@kb-srv.ysn.ru
uname -a
Linux web-hosting 4.18.0-497.el8.x86_64 #1 SMP Sat Jun 10 12:24:53 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
```

### Сборка для ubuntu 18.04 (bionic)

Скачать образ в докер
```
docker create ubuntu:18.04
```
Запустить в докере оболочку
```
docker create ubuntu:18.04
docker run -itv /home/andrei/src:/home/andrei/src ubuntu:18.04 bash
```
Обновить репозиторий и установить пакеты
```
apt install lsb-core software-properties-common git build-essential wget mc gcc clang  
 libc-ares-dev odb libodb-sqlite-2.4 libmicrohttpd-dev gettext sqlite3 libsqlite3-dev libssl-dev
```

// apt remove libgrpc++-dev libgrpc-dev protobuf-compiler-grpc protobuf-compiler libprotobuf-dev

Проверьте версию компилятора odb

```
odb --version
ODB object-relational mapping (ORM) compiler for C++ 2.4.0
```

Замените GCC на более раннюю версию 

[ODB GCC7.5.0 -mtune bug](https://stackoverflow.com/questions/60762771/what-causes-gcc-invalid-option-for-mtune)

```
apt-get install gcc-7-base=7.3.0-16ubuntu3 cpp-7=7.3.0-16ubuntu3 gcc-7=7.3.0-16ubuntu3 libgcc-7-dev=7.3.0-16ubuntu3 libasan4=7.3.0-16ubuntu3 libubsan0=7.3.0-16ubuntu3 libcilkrts5=7.3.0-16ubuntu3
apt-get install g++-7=7.3.0-16ubuntu3 libstdc++-7-dev=7.3.0-16ubuntu3 
ln -s /usr/bin/g++-7 /usr/bin/g++
ln -s /usr/bin/gcc-7 /usr/bin/gcc
ln -s /usr/bin/gcc /usr/bin/cc
```

Установите новый CMake (в репозитории старый)):
```
wget -c https://github.com/Kitware/CMake/releases/download/v3.30.4/cmake-3.30.4-linux-x86_64.sh
chmod a+x cmake-3.30.4-linux-x86_64.sh
./cmake-3.30.4-linux-x86_64.sh
ln -s /usr/local/bin/cmake /usr/bin/cmake
```
или соберите CMake вручную:
```
cd /home/andrei/src/git
wget -c https://github.com/Kitware/CMake/releases/download/v3.30.4/cmake-3.30.4.tar.gz
tar xvfz cmake-3.30.4.tar.gz
cd cmake-3.30.4
./configure
make
sudo make install
```

Установите bazel (необязательно):
```
wget -c https://github.com/bazelbuild/bazel/releases/download/7.3.2/bazel-7.3.2-installer-linux-x86_64.sh
chmod a+x bazel-7.3.2-installer-linux-x86_64.sh
./bazel-7.3.2-installer-linux-x86_64.sh
```
Соберите libicu вручную (в репозитории версия 60.2 а нужна >=61.0 и не выше 73):
```
cd /home/andrei/src/git
# git clone https://github.com/unicode-org/icu.git
wget -c https://github.com/unicode-org/icu/archive/refs/tags/release-73-2.tar.gz
tar xvfz release-73-2.tar.gz
cd icu-release-73-2/icu4c/source
./configure
make
sudo make install
```

Или скачать zip файл, распаковать, собрать и установить
```
wget -c https://codeload.github.com/unicode-org/icu/zip/refs/heads/main
cd icu-main/icu4c/source
./configure
make
sudo make install
```

Соберите Xlnt вручную:

```
cd /home/andrei/src/git
git clone https://github.com/tfussell/xlnt.git
cd xlnt
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make
make install
ls /usr/local/lib/libxlnt.so.1.5.0
ls /usr/local/include/xlnt/xlnt.hpp
```

Соберите absl с поддержкой C++14 вручную:

```
cd /home/andrei/src/git
git clone https://github.com/abseil/abseil-cpp.git
cd /home/andrei/src/git/abseil-cpp
mkdir -p build
cd build
cmake -DCMAKE_CXX_STANDARD=14 -DCMAKE_CXX_FLAGS='-D_GLIBCXX_USE_CXX11_ABI=0 ..
make
make install
```

Соберите grpc++ версии 1.56 вручную:

```
cd /home/andrei/src/git
git clone --recurse-submodules -b v1.56.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
cd  cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_CXX_STANDARD=14 ../..
make -j 1
make install
```

Если protoc автоматически не собрался, соберите protoc вручную:

```
cd /home/andrei/src/git/grpc/third_party/protobuf
mkdir -p build
cd build
cmake ..
make
make install
```

Очистите и сгененируйте файлы в папке gen заново
```
cd gen
rm *
cd ..
tools/generate-code
```
Запустите сборку
```
cd /home/andrei/src/rcr
mkdir -p build-ubuntu18
cd build-ubuntu18
cmake ..
make
```

### Зависимости

/mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by ./mkdb)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.32' not found (required by ./mkdb)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by ./mkdb)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by libprotobuf.so.23)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libprotobuf.so.23)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libodb-2.4.so)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.26' not found (required by libodb-sqlite-2.4.so)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by libodb-sqlite-2.4.so)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.32' not found (required by libgrpc++.so.1)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by libgrpc++.so.1)
./mkdb: /lib64/libm.so.6: version `GLIBC_2.29' not found (required by libicuuc.so.70)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.33' not found (required by libicuuc.so.70)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libicuuc.so.70)
./mkdb: /lib64/libm.so.6: version `GLIBC_2.29' not found (required by libgrpc.so.10)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by libgrpc.so.10)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.32' not found (required by libgrpc.so.10)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libgrpc.so.10)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.33' not found (required by libgrpc.so.10)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libgpr.so.10)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.33' not found (required by libcrypto.so.3)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libcrypto.so.3)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by libabsl_str_format_internal.so.20210324)
./mkdb: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found (required by libabsl_time_zone.so.20210324)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.32' not found (required by libabsl_base.so.20210324)
./mkdb: /lib64/libc.so.6: version `GLIBC_2.34' not found (required by libabsl_base.so.20210324)


```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/var/www/clients/client2/web48/home/nocmicroadmin/src
```

```
scp mkdb rcr-cli rcr-svc nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libprotobuf.so.23 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libodb-2.4.so nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libodb-sqlite-2.4.so nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libgrpc++.so.1 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libicuuc.so.70 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_strings.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libgrpc.so.10 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libgpr.so.10 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libicudata.so.70 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_strings_internal.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_int128.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_throw_delegate.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_raw_logging_internal.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libssl.so.3 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libcrypto.so.3 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_time.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_bad_optional_access.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_str_format_internal.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_time_zone.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_base.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
scp /usr/lib/x86_64-linux-gnu/libabsl_spinlock_wait.so.20210324 nocmicroadmin@micro.ikfia.ysn.ru:/var/www/clients/client2/web48/home/nocmicroadmin/src
```

## Бан пользователей

В таблице User поставьте в записи пользователя значение поля token равным 0.

### Запуск на kb-srv.ikfia.ru

Скопировать бинарные файлы, файл базы данных и плагин(ы)
```
export LANG=ru_RU.UTF-8
./mddb
strip rcr-cli rcr-svc mkdb box
scp rcr-cli rcr-svc mkdb box rcr.db user@kb-srv.ysn.ru:~/rcr
scp liblogin-ad.so user@kb-srv.ysn.ru:~/rcr/plugins
```

Скопировать веб-страницу
```
scp /home/andrei/src/rct-web/dist/rcr/* user@kb-srv.ysn.ru:/var/www/html
```
Запустить с указанием URL сервера домена (Active Directory)
```
ssh user@kb-srv.ysn.ru
export LANG=ru_RU.UTF-8
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/rcr
./rcr-svc -jvd -u plugins -U "ldap://ad.ysn.ru"

или

./rcr-svc -jvd -u plugins -U "ldap://ad.ysn.ru" -l 50051 -p 8050 --db kb.db
./rcr-svc -jvd -u plugins -U "ldap://ad.ysn.ru" -l 50052 -p 8052 --db lmii.db

```

## Баги

На kb-srv при переключении пользователя возникает ошибка 
```
kernel: [7296666.659926] MHD-worker[228290]: segfault at 55795c735 ip 00007f93333f42e5 sp 00007f93331b7f70 error 4 in libldap-2.5.so.0.1.10[7f93333e7000+3a000]
```
Кажется, устранен

## Вспомогательные скрипты

### Запуск

start-rcr.sh
```
#!/bin/sh
H=/home/user/rcr
export LANG=ru_RU.UTF-8;export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$H
$H/rcr-svc -jvd --pidfile /var/run/rcr.pid -u plugins -U "ldap://ad.ysn.ru" -l 50051 -P 8050 --db $H/kb.db
#./rcr-svc -jvd -p /var/run/lmii.pid -u plugins -U "ldap://ad.ysn.ru" -l 50052 -P 8052 --db lmii.db
exit 0
```

### Сервис Systemd

etc/systemd/system/rcr.service

```
[Unit]
Description=RCR web service

[Service]
Type=forking
# The PID file is optional, but recommended in the manpage
# "so that systemd can identify the main process of the daemon"
PIDFile=/var/run/rcr.pid
ExecStart=/home/user/start-rcr.sh
Restart=on-abort

[Install]
WantedBy=multi-user.target
```

Создать сервис

```
sudo systemctl enable rcr.service
sudo systemctl daemon-reloa
```

Запустить сервис

```
sudo systemctl start rcr.service
sudo systemctl status rcr.service
```

Остановить сервис

```
sudo systemctl stop rcr.service
sudo systemctl status rcr.service
```

## Cleanup

```
DELETE FROM 'Card';
DELETE FROM 'Property';
DELETE FROM 'Package';
DELETE FROM 'Box';
DELETE FROM 'Journal';

UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Card';
UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Property';
UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Package';
UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Box';
UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Journal';
```