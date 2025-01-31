#include <assert.h>
#include <ev_buffered_stream.h>

ev_buffered_stream::ev_buffered_stream(chunked_memory_stream * memory_stream, std::streamsize bufsize, std::streamsize max_len):
		_bufsize(bufsize),
		_p_buffer(0),
		_w_buffer(0),
		_mode(ios::out|ios::in),
		_memory_stream(memory_stream),
		_nodeptr(0),
		_prev_len(0),
		_max_len(max_len),
		_cum_len(0),
		_prefix_len(0),
		_suffix_len(0),
		_consume_all_of_max_len(0),
		_reading_buf_from_source(0)
{
	this->setg(_p_buffer, _p_buffer, _p_buffer);
	if(!_w_buffer) {
		_w_buffer = (char*)malloc(BUFFER_SIZE);
		memset(_w_buffer,0,BUFFER_SIZE);
	}

	this->setp(_w_buffer+_prefix_len, _w_buffer+BUFFER_SIZE-_suffix_len);
	if (0 == _max_len) _max_len = -1;
}

void ev_buffered_stream::set_prefix_len(size_t bytes)
{
	if (bytes > 6) std::abort();
	_prefix_len = bytes;
	this->setp(_w_buffer+_prefix_len, _w_buffer+BUFFER_SIZE-_suffix_len);
}

void ev_buffered_stream::set_suffix_len(size_t bytes)
{
	if (bytes > 4) std::abort();
	_suffix_len = bytes;
	this->setp(_w_buffer+_prefix_len, _w_buffer+BUFFER_SIZE-_suffix_len);
}

void ev_buffered_stream::consume_all_of_max_len()
{
	_consume_all_of_max_len = 1;
}

ev_buffered_stream::~ev_buffered_stream()
{
	ssize_t erase_size = 0;

	/* 
	 * If all of the data upto max_len has to be consumed,
	 * it has to be erased from the memory_stream.
	 */
	if ((_consume_all_of_max_len) && (_mode & ios::in) && (_max_len > 0)) {
		while (low_read_from_source(_bufsize));
	}

	flush_buffer();
	free(_w_buffer);
	_w_buffer = NULL;
}

int ev_buffered_stream::overflow(int c)
{
	if (!(_mode & ios::out)) return EOF;

	if (flush_buffer() == std::streamsize(-1)) return EOF;
	if (c != EOF) {
		*this->pptr() = (char)(c);
		this->pbump(1);
	}

	return c;
}

int ev_buffered_stream::underflow()
{
	if (!(_mode & ios::in)) return EOF;

	if (this->gptr() && (this->gptr() < this->egptr()))
		return int(*this->gptr());

	// n will be <= _bufsize
	size_t n = high_read_from_source(_bufsize);
	assert(n <=_bufsize);
	if (n <= 0) return EOF;

	this->setg(_p_buffer, _p_buffer, _p_buffer + n);

	// return next character
	return (int)(*this->gptr());    
}

int ev_buffered_stream::sync()
{
	if (this->pptr() && this->pptr() > this->pbase()) {
		if (flush_buffer() == -1) return -1;
	}

	return 0;
}


void ev_buffered_stream::set_mode(openmode mode)
{
	_mode = mode;
}

ev_buffered_stream::openmode ev_buffered_stream::get_mode() const
{
	return _mode;
}

int ev_buffered_stream::low_readch()
{
	char c = 0;
	int ch = EOF;
	if (!_reading_buf_from_source) std::abort();

	_memory_stream->erase(_prev_len);
	_prev_len = 0;
	if ((-1 == _max_len) || (_cum_len < _max_len)) {
		if (!(_memory_stream->read(&c, 1))) return EOF;
		_cum_len += 1;
		ch = c;
	}

	return ch;
}

size_t ev_buffered_stream::low_read_from_source(std::streamsize size)
{
	size_t len = 0;

	_memory_stream->erase(_prev_len);
	if ((-1 == _max_len) || (_cum_len < _max_len)) {


		if ((-1 != _max_len) && ((_cum_len + size) > _max_len))
			size = _max_len - _cum_len;

		_nodeptr = _memory_stream->get_next(0);

		if(_nodeptr) {
			len = _memory_stream->get_buffer_len(_nodeptr);
			_p_buffer = (char*)_memory_stream->get_buffer(_nodeptr);
			/* This is in order to limit the size read from the source
			 * So that if there is any logic by the caller that after certain
			 * length the stream should return EOF, the stream buffer
			 * wil behave accordingly.
			 * */
			if (len < 0) len = 0;
			if (len > size) len = size;
		}
		else {
			_p_buffer = NULL;
			len = 0;
		}

		_cum_len += len;
	}
	_prev_len = len;
	return len;
}

size_t ev_buffered_stream::read_from_source(std::streamsize size)
{
	return low_read_from_source(size);
}

size_t ev_buffered_stream::high_read_from_source(std::streamsize size)
{
	size_t s = 0;
	_reading_buf_from_source = 1;
	s = read_from_source(size);
	_reading_buf_from_source = 0;
	return s;
}

size_t ev_buffered_stream::push_to_sync(char *buffer, std::streamsize size)
{
	size_t len = size;
	if ((-1 == _max_len) || (_cum_len < _max_len)) {
		if ((-1 != _max_len) && ((_cum_len + len) > _max_len))
			len = _max_len - _cum_len;

		_memory_stream->push(buffer, len);

		_cum_len += len;
	}
	return len;
}

void ev_buffered_stream::get_prefix(char* buffer, std::streamsize bytes, char *buffer_ptr, size_t bytes_ptr)
{
	return;
}

void ev_buffered_stream::get_suffix(char* buffer, std::streamsize bytes, char *buffer_ptr, size_t bytes_ptr)
{
	return;
}

size_t ev_buffered_stream::low_write_to_sync(char *buffer, std::streamsize bytes)
{
	size_t transfered = 0;
	size_t size = 0;
	size = bytes;
	if(_prefix_len) {
		char prefix[10] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
		get_prefix(buffer+_prefix_len, bytes, prefix, _prefix_len);
		if (strlen(prefix) != _prefix_len) std::abort();
		memcpy(buffer, prefix, _prefix_len);
		size += _prefix_len;
	}
	if (_suffix_len) {
		char suffix[10] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
		get_suffix(buffer+_prefix_len, bytes, suffix, _suffix_len);
		if (strlen(suffix) != _suffix_len) std::abort();
		memcpy(buffer+size, suffix, _suffix_len);
		size += _suffix_len;
	}

	transfered += push_to_sync(buffer, size);

	return transfered;
}

size_t ev_buffered_stream::write_to_sync(char *buffer, std::streamsize bytes)
{
	return low_write_to_sync(buffer, bytes);
}

int ev_buffered_stream::flush_buffer()
{
	if (!(_mode & ios::out)) return EOF;
	if (!_w_buffer) return -1;

	int n = int(this->pptr() - this->pbase());

	if (n) {

		if (write_to_sync(_w_buffer, n) >= n) {
			_w_buffer = (char*)malloc(BUFFER_SIZE);
			memset(_w_buffer,0,BUFFER_SIZE);
			this->setp(_w_buffer+_prefix_len, _w_buffer+BUFFER_SIZE-_suffix_len);
			return n;
		}
	}

	return -1;
}
