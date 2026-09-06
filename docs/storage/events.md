# Event storage (Phase 8)

`EventWriter` persists `EventLog` increments on a dedicated `std::jthread`.
The GUI thread must not run `QSqlQuery`.

## Path

Production `CameraManager()` opens

`QStandardPaths::AppDataLocation` / `events.sqlite`

(organization `VisionLab`, application `VisionLab`). Tests must call
`CameraManager::setEventDatabasePath` with a `QTemporaryDir` path.
Do not assert against the developer home directory.

## Writer thread

- Queue capacity **1024**, `DropOldest`. `enqueue` never blocks on SQL.
- The writer `insert`s then `prune(10000)`.
- `requestQuery` shares the same queue so query and insert stay on one
  connection / one thread. Results return via `QMetaObject::invokeMethod`
  (`Qt::QueuedConnection`) to the receiver's thread.
- `start` failure (`open` false): `qWarning`, later `enqueue` is a no-op.
- Shutdown order: `pipeline->stop()` then `writer.stop()` (close queue,
  join, close repo).

## Session cursor

`VisionPipeline::start` resets `EventLog` so `eventId` begins at 1.
`CameraManager::start` resets the persisted max id after a successful
pipeline start. Otherwise the new session's id=1 would be skipped.

## Known limit

`EventLog` stays at **256** (DropOldest). If the GUI stalls, events that
never reached `notifyFrame` are not persisted. This phase does not
enlarge the log.
