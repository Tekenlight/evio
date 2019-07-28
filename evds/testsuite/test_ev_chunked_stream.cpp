#include <chunked_memory_stream.h>
#include <string.h>


static void reader(char * string, chunked_memory_stream & cms, size_t offset, size_t length)
{
	int n = 0;
	memset(string,'\0',10);
	n = cms.read(offset, string, length);
	//printf("Read %d bytes from offset [%lu] and string is [%s]\n", n, offset, string);

	return ;
}

static void test_main(CuTest *tc)
{
	int ret = 0;
	char string[10];
	char * a = (char*)calloc(1, 2);
	char * b = (char*)calloc(1, 2);
	char * c = (char*)calloc(1, 2);
	char * d = (char*)calloc(1, 2);
	chunked_memory_stream cms;

	memcpy(a, "01", 2);
	cms.push(a, 2);

	memcpy(b, "23", 2);
	cms.push(b, 2);

	memcpy(c, "45", 2);
	cms.push(c, 2);

	memcpy(d, "67", 2);
	cms.push(d, 2);

	reader(string, cms, 0, 3);
	CuAssertTrue(tc, !strcmp("012",string));

	reader(string, cms, 1, 1);
	CuAssertTrue(tc, !strcmp("1",string));

	reader(string, cms, 3, 5);
	CuAssertTrue(tc, !strcmp("34567",string));

	reader(string, cms, 3, 6);
	CuAssertTrue(tc, !strcmp("34567",string));

	ret = reader(string, cms, 8, 6);
	CuAssertTrue(tc, (ret == -1));

	cms.erase(5);
	reader(string, cms, 0, 8);
	CuAssertTrue(tc, !strcmp("567",string));

	return;
}

static int run_ev_test()
{
    CuSuite* suite = CuSuiteNew();
    CuString *output = CuStringNew();

    SUITE_ADD_TEST(suite,test_main);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    printf("Count = %d\n",suite->count);
    return suite->failCount;
}

int main(int argc, char* argv[])
{
	return run_ev_test();
}