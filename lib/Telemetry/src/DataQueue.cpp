#include "DataQueue.h"

DataQueue::DataQueue() {
	_init();
}

void DataQueue::add(String id, int value) {
	_writer->beginObject()
    	.name("t").value(id)
    	.name("d").value(value)
    .endObject();
}

void DataQueue::add(String id, String value) {
	_writer->beginObject()
    	.name("t").value(id)
    	.name("d").value(value)
    .endObject();
}

void DataQueue::add(String id, float value) {
	_writer->beginObject()
    	.name("t").value(id)
    	.name("d").value(value)
    .endObject();
}

void DataQueue::loop() {
	_publishQueuePosix->loop();
}

String DataQueue::publish(String event, PublishFlags flag1, PublishFlags flag2) {
	String payload = _writerGet();
	// _publishQueue->publish(event, payload, flag1, flag2);
	_publishQueuePosix->publish(event, payload, flag1, flag2);
	_writerRefresh();
    return payload;
}

String DataQueue::resetData() {
    String payload = _writerGet();
    _writerRefresh();
    return payload;
}

void DataQueue::_writerRefresh() {
	if(this->_writer != NULL) delete this->_writer;
    _writerInit();
}

String DataQueue::_writerGet() {
	_writer->endArray().endObject();
    return String(_buf);
}

void DataQueue::_writerInit() {
    memset(_buf, 0, sizeof(_buf));
	this->_writer = new JSONBufferWriter(_buf, sizeof(_buf) - 1);

	_writer->beginObject()
		.name("time").value((int)Time.now())
		.name("d").beginArray();
}

void DataQueue::_init() {
	_publishQueuePosix = &(PublishQueuePosix::instance());
	_publishQueuePosix->setup();
	_publishQueuePosix->withRamQueueSize(RAM_QUEUE_EVENT_COUNT)
		.withDirPath(POSIX_DIRECTORY_PATH);

	// Initialize Queue
	_writerInit();
}