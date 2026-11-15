# SeniorSphere — simple C++ backend

## What to explain in your viva

SeniorSphere lets juniors ask questions, chat with other students and arrange mentorship. React handles the screens. A C++ server checks requests, updates C++ objects and saves them in one JSON file.

Request flow: React → controller → model/repository → JSON file → JSON response.

Example: sending a message
1. React sends POST /api/conversations/1/messages with the login token and message.
2. ChatController checks the token and confirms that the student belongs to that conversation.
3. It creates a Message object and adds it to Repository<Message>.
4. Database saves the collections to disk.
5. The recipient's browser fetches updated conversations every three seconds.

## Classes and OOP concepts

| Concept | Concrete example |
|---|---|
| Class/object | User, Question, Message, Conversation, Booking |
| Encapsulation | User keeps identity and password fields private; Conversation controls read markers |
| Abstraction | Entity defines toJson/fromJson; Controller defines registerRoutes |
| Inheritance | User and Question inherit Entity; feature controllers inherit Controller |
| Polymorphism | main stores different controllers in unique_ptr<Controller> and calls registerRoutes |
| Templates | Repository<T> stores any Entity-derived type, avoiding repeated collection code |
| Composition | Database owns repositories; Question owns answers |
| Exceptions | HttpError carries a status and readable error message |
| File handling | Database loads and saves db.json |
| STL | deque, vector, map, set and sorting |

Do not claim multiple inheritance or Entity equality operator overloading: this version does not use them. Some model fields are public, so encapsulation is partial rather than universal.

## What libraries do

- cpp-httplib 0.66.0 handles sockets, HTTP parsing and routing.
- nlohmann/json 3.12.0 parses and writes JSON. Json.h is a small compatibility interface for the feature classes, not a custom parser.
- libsodium hashes new passwords with Argon2id and produces secure random session tokens. The library manages the salt and hashing parameters.

You should understand why these libraries are needed, but you do not need to present their internals as your own code. Old salted SHA-256 account hashes remain readable for compatibility; the supplied database contains no real accounts to migrate. New registrations use Argon2id.

The server uses eight network workers. A single lock allows one business operation at a time, keeping in-memory collections and slot claims consistent. Socket reads happen outside that lock. This is suitable for a small project, not a high-traffic service.

## Fixes in this revision

- Anonymous doubts cannot receive unsolicited private bookings. The author can choose to message a mentor. Legacy anonymous booking responses also hide the author's ID.
- Profile updates validate a copy before applying it, and roll back if saving fails.
- Database replacement preserves the old file until the replacement succeeds. Linux flushes the file before rename; Windows uses a replace/write-through operation.
- HTTP and JSON parsing use libraries; custom SHA-256 code is removed.
- Session tokens expire after 24 hours, and logout invalidates a token. Restarting the server signs users out.
- CORS accepts explicitly configured frontend origins instead of every website.
- Demo data is opt-in. Production starts empty.
- Network reads have timeouts, request bodies have a size limit and JSON nesting is limited.

## Features actually implemented

Signup/login/logout, fixed signup identity, editable profile, student directory, connections, private chat with unread counts, questions and answers, question upvotes, open mentorship slots, help requests, bookings and notifications.

Answer acceptance, answer upvotes and booking cancellation are not implemented. Basic bookings are immediately marked Confirmed. Do not claim those missing features in the report. The directory API returns student profiles; React filters the visible seniors. Mentorship slot posting is restricted to years 3–4.

## Running and deployment

See the repository README for current setup and deployment steps. The public seed is intentionally empty and compiled binaries are not tracked.
