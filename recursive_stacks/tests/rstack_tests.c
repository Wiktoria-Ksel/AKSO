#ifdef NDEBUG
#undef NDEBUG
#endif

#include "rstack.h"
#include "memory_tests.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>

/** MAKRA SKRACAJĄCE IMPLEMENTACJĘ TESTÓW **/

// To są możliwe wyniki testu.
#define PASS 0
#define FAIL 1
#define WRONG_TEST 2

// Oblicza liczbę elementów tablicy x.
#define SIZE(x) (sizeof x / sizeof x[0])

#define ASSERT(f)  \
  do {             \
    if (!(f))      \
      return FAIL; \
  } while (0)

#define ASSERT_RESULT(c, f, ...)          \
  do {                                    \
    result_t r = c;                       \
    if (r.flag != (f))                    \
      return FAIL;                        \
    if ((f) && r.value != __VA_ARGS__ -0) \
      return FAIL;                        \
  } while (0)

#define ASSERT_VARIANT(c, v1, v2)                              \
  do {                                                         \
    result_t r = c;                                            \
    if (r.flag != false && r.value != (v1) && r.value != (v2)) \
      return FAIL;                                             \
  } while (0)

#define CHECK_IF_NO_ERROR(f) \
  do {                       \
    if ((f) != 0)            \
      return FAIL;           \
  } while (0)

#define CHECK_ERRNO(f, r, e)        \
  do {                              \
    errno = 0;                      \
    if ((f) != (r) || errno != (e)) \
      return FAIL;                  \
  } while (0)

#define CHECK_ERRNO_3(f, r, e1, e2, e3)                                     \
  do {                                                                      \
    errno = 0;                                                              \
    if (!((f) == (r) && (errno == (e1) || errno == (e2) || errno == (e3)))) \
      return FAIL;                                                          \
  } while (0)

#define V(code, where) (((unsigned long)code) << (3 * where))

/** WŁAŚCIWE TESTY **/

static int zero(void) {
  rstack_t *rs0 = rstack_new();
  assert(rs0);

  ASSERT(rstack_empty(rs0) == true);
  ASSERT_RESULT(rstack_front(rs0), false);
  CHECK_IF_NO_ERROR(rstack_write("file_zero.out", rs0));
  rstack_delete(rs0);

  return PASS;
}

static int one(void) {
  rstack_t *rs0 = rstack_new();
  assert(rs0);
  rstack_t *rs1 = rstack_new();
  assert(rs1);

  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 1));
  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 2));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 0));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs1, rs0));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 3));
  ASSERT(rstack_empty(rs0) == false);
  ASSERT_RESULT(rstack_front(rs0), true, 3);
  rstack_delete(rs0);
  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 4));
  CHECK_IF_NO_ERROR(rstack_write("file_one.out", rs1));
  ASSERT(rstack_empty(rs1) == false);
  ASSERT_RESULT(rstack_front(rs1), true, 4);
  rstack_delete(rs1);

  return PASS;
}

static int two(void) {
  rstack_t *rs1 = rstack_new();
  assert(rs1);
  rstack_t *rs2 = rstack_new();
  assert(rs2);

  CHECK_IF_NO_ERROR(rstack_push_rstack(rs1, rs2));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
  ASSERT(rstack_empty(rs1) == true);
  ASSERT(rstack_empty(rs2) == true);
  ASSERT_RESULT(rstack_front(rs1), false);
  ASSERT_RESULT(rstack_front(rs2), false);
  rstack_delete(rs1);
  rstack_delete(rs2);

  rs2 = rstack_new();
  assert(rs2);

  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 4));
  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 5));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs2));
  ASSERT(rstack_empty(rs2) == false);
  ASSERT_RESULT(rstack_front(rs2), true, 5);
  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 6));
  CHECK_IF_NO_ERROR(rstack_write("file_two.out", rs2));
  ASSERT(rstack_empty(rs2) == false);
  ASSERT_RESULT(rstack_front(rs2), true, 6);
  rstack_delete(rs2);

  rs1 = rstack_new();
  assert(rs1);
  rs2 = rstack_new();
  assert(rs2);

  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 11));
  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 12));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs1, rs2));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
  ASSERT_VARIANT(rstack_front(rs1), 11, 12);
  rstack_delete(rs1);
  rstack_delete(rs2);

  rstack_t *rsa = rstack_new();
  assert(rsa);
  rstack_t* rsb = rstack_new();
  assert(rsb);
  rstack_t* rsc = rstack_new();
  assert(rsc);

  CHECK_IF_NO_ERROR(rstack_push_rstack(rsc, rsa));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsc, rsb));
  CHECK_IF_NO_ERROR(rstack_push_value(rsb, 2));
  CHECK_IF_NO_ERROR(rstack_push_value(rsa, 1));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsb, rsa));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsa, rsb));
  ASSERT_VARIANT(rstack_front(rsc), 1, 2);

  rstack_delete(rsa);
  rstack_delete(rsb);
  rstack_delete(rsc);

  rsa = rstack_new();
  assert(rsa);
  rsb = rstack_new();
  assert(rsb);

  CHECK_IF_NO_ERROR(rstack_push_value(rsa, 101));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsa, rsb));
  CHECK_IF_NO_ERROR(rstack_push_value(rsb, 102));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsb, rsb));
  ASSERT_VARIANT(rstack_front(rsa), 102, 102);

  rstack_delete(rsa);
  rstack_delete(rsb);

  rsa = rstack_new();
  assert(rsa);
  rsb = rstack_new();
  assert(rsb);

  CHECK_IF_NO_ERROR(rstack_push_value(rsa, 201));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsa, rsb));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rsb, rsb));
  ASSERT_VARIANT(rstack_front(rsa), 201, 201);

  rstack_delete(rsa);
  rstack_delete(rsb);

  return PASS;
}

static int three(void) {
  rstack_t *rs0 = rstack_new();
  assert(rs0);
  rstack_t *rs3 = rstack_new();
  assert(rs3);

  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 7));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs3, rs0));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs3, rs0));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs3, rs0));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 8));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 9));
  ASSERT(rstack_empty(rs3) == false);
  ASSERT(rstack_empty(rs0) == false);
  ASSERT_RESULT(rstack_front(rs3), true, 9);
  ASSERT_RESULT(rstack_front(rs0), true, 9);
  rstack_pop(rs3);
  rstack_pop(rs0);
  CHECK_IF_NO_ERROR(rstack_write("file_three.out", rs3));
  CHECK_IF_NO_ERROR(rstack_write("file_three.out", rs3));
  CHECK_IF_NO_ERROR(rstack_write("file_three.out", rs3));
  ASSERT(rstack_empty(rs3) == false);
  ASSERT(rstack_empty(rs0) == false);
  ASSERT_RESULT(rstack_front(rs3), true, 8);
  ASSERT_RESULT(rstack_front(rs0), true, 8);
  rstack_delete(rs3);
  rstack_delete(rs0);

  return PASS;
}

static int four(void) {
  rstack_t *rs4 = rstack_read("file_four.in");
  assert(rs4);

  CHECK_IF_NO_ERROR(rstack_write("file_four.out", rs4));
  rstack_delete(rs4);

  return PASS;
}

static int five(void) {
  rstack_t *rs5 = rstack_new();
  assert(rs5);
  rstack_t *rs6 = rstack_new();
  assert(rs6);
  rstack_t *rs7 = rstack_new();
  assert(rs7);

  CHECK_IF_NO_ERROR(rstack_push_value(rs5, 5));
  CHECK_IF_NO_ERROR(rstack_push_value(rs6, 6));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs6, rs6));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs7, rs7));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs6, rs7));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs5, rs6));
  ASSERT(rstack_empty(rs5) == false);
  ASSERT(rstack_empty(rs6) == false);
  ASSERT(rstack_empty(rs7) == true);
  ASSERT_RESULT(rstack_front(rs5), true, 6);
  ASSERT_RESULT(rstack_front(rs6), true, 6);
  ASSERT_RESULT(rstack_front(rs7), false);
  CHECK_IF_NO_ERROR(rstack_write("file_five.out", rs5));
  rstack_delete(rs5);
  rstack_delete(rs6);
  rstack_delete(rs7);

  return PASS;
}

static int big(void) {
  rstack_t *rs0[65536];
  for (size_t i = 0; i < SIZE(rs0); ++i) {
    rs0[i] = rstack_new();
    ASSERT(rs0[i]);
    CHECK_IF_NO_ERROR(rstack_push_value(rs0[i], (uint64_t)i));
  }

  const size_t idx = 777;
  const uint64_t v_max = 270000;
  for (uint64_t v = 1; v <= v_max; ++v) {
    CHECK_IF_NO_ERROR(rstack_push_value(rs0[idx], v));
    ASSERT_RESULT(rstack_front(rs0[idx]), true, v);
  }
  for (uint64_t v = v_max - 1; v > 0; --v) {
    rstack_pop(rs0[idx]);
    ASSERT_RESULT(rstack_front(rs0[idx]), true, v);
    ASSERT(rstack_empty(rs0[idx]) == false);
  }
  rstack_pop(rs0[idx]);
  ASSERT_RESULT(rstack_front(rs0[idx]), true, idx);
  rstack_pop(rs0[idx]);
  ASSERT_RESULT(rstack_front(rs0[idx]), false);

  rstack_t *rs1 = rstack_new();
  ASSERT(rs1);
  for (uint64_t v = 0; v < 24; ++v) {
    rstack_t *rs2 = rstack_new();
    ASSERT(rs2);
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    rstack_delete(rs1);
    rs1 = rs2;
  }
  ASSERT(rstack_empty(rs1) == true);
  ASSERT_RESULT(rstack_front(rs1), false);
  rstack_t *rs2 = rstack_new();
  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 22));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
  ASSERT(rstack_empty(rs2) == false);
  ASSERT_RESULT(rstack_front(rs2), true, 22);
  rstack_delete(rs1);
  rstack_delete(rs2);

  rs1 = rstack_new();
  ASSERT(rs1);
  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 1));
  for (uint64_t v = 2; v <= 128; ++v) {
    rstack_t *rs2 = rstack_new();
    ASSERT(rs2);
    CHECK_IF_NO_ERROR(rstack_push_value(rs2, v));
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    rstack_delete(rs1);
    rs1 = rs2;
  }
  ASSERT(rstack_empty(rs1) == false);
  ASSERT_RESULT(rstack_front(rs1), true, 1);
  rstack_delete(rs1);

  rs1 = rstack_new();
  ASSERT(rs1);
  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 1));
  for (uint64_t v = 2; v <= 16; ++v) {
    rstack_t *rs2 = rstack_new();
    ASSERT(rs2);
    CHECK_IF_NO_ERROR(rstack_push_value(rs2, v));
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
    rstack_delete(rs1);
    rs1 = rs2;
  }
  ASSERT(rstack_empty(rs1) == false);
  ASSERT_RESULT(rstack_front(rs1), true, 1);
  CHECK_IF_NO_ERROR(rstack_write("file_big.out", rs1));
  rstack_delete(rs1);

  for (size_t i = 0; i < SIZE(rs0); ++i)
    rstack_delete(rs0[i]);

  return PASS;
}

static int cycles(void) {
  rstack_t *rs[7];
  const size_t size = SIZE(rs);

  for (size_t i = 0; i < size; ++i)
    ASSERT((rs[i] = rstack_new()) != nullptr);
  for (size_t i = 0; i < size; ++i)
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs[i], rs[(i + 1) % size]));
  for (size_t i = 0; i < size; ++i)
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs[i], rs[(i + 3) % size]));
  for (size_t i = 0; i < size; ++i)
    CHECK_IF_NO_ERROR(rstack_push_rstack(rs[i], rs[(i + 5) % size]));
  for (size_t i = 0; i < size; ++i)
    rstack_pop(rs[i]);
  for (size_t i = 0; i < size; ++i)
    rstack_delete(rs[i]);

  return PASS;
}

static int params(void) {
  rstack_t *rs = rstack_new();
  assert(rs);

  rstack_delete(nullptr);

  CHECK_ERRNO(rstack_push_value(nullptr, 0), -1, EINVAL);
  CHECK_ERRNO(rstack_push_rstack(nullptr, rs), -1, EINVAL);
  CHECK_ERRNO(rstack_push_rstack(rs, nullptr), -1, EINVAL);

  rstack_pop(rs);
  rstack_pop(nullptr);

  ASSERT(rstack_empty(rs));
  ASSERT(rstack_empty(nullptr));

  ASSERT_RESULT(rstack_front(rs), false);
  ASSERT_RESULT(rstack_front(nullptr), false);

  CHECK_ERRNO(rstack_read(nullptr), nullptr, EINVAL);
  CHECK_ERRNO(rstack_write(nullptr, rs), -1, EINVAL);
  CHECK_ERRNO(rstack_write("nazwa_pliku", nullptr), -1, EINVAL);

  rstack_delete(rs);

  return PASS;
}

// Zlicza deskryptory otwartych plików.
static int open_files_count(void) {
  int count = 0;
  for (int fd = 0; fd < FD_SETSIZE; ++fd)
    count += fcntl(fd, F_GETFD) >= 0;
  return count;
}

static int parse(void) {
  int n = open_files_count();

  rstack_t *rs = rstack_read("file_parse_good_0.in");
  ASSERT(rs);
  ASSERT(rstack_empty(rs));
  rstack_delete(rs);

  rs = rstack_read("file_parse_good_1.in");
  ASSERT(rs);
  ASSERT_RESULT(rstack_front(rs), true, 10000000000000999999UL);
  ASSERT(rstack_empty(rs) == false);
  rstack_delete(rs);

  rs = rstack_read("file_parse_good_2.in");
  ASSERT(rs);
  ASSERT_RESULT(rstack_front(rs), true, UINT64_MAX);
  rstack_pop(rs);
  ASSERT_RESULT(rstack_front(rs), true, 1234567890UL);
  rstack_pop(rs);
  ASSERT(rstack_empty(rs));
  rstack_delete(rs);

  for (unsigned i = 1; i <= 9; ++i) {
    char file[24];
    int r = snprintf(file, sizeof file, "file_parse_bad_%u.in", i);
    assert(r > 0 && (size_t)r < sizeof file);
    if (i == 3 || i == 4 || i == 5)
      CHECK_ERRNO_3(rstack_read(file), nullptr, EINVAL, ERANGE, EOVERFLOW);
    else
      CHECK_ERRNO(rstack_read(file), nullptr, EINVAL);
  }

  ASSERT(n == open_files_count());

  return PASS;
}

static int read_error(void) {
  int n = open_files_count();

  CHECK_ERRNO(rstack_read("nie_ma_takiego_pliku"), nullptr, ENOENT);
  CHECK_ERRNO(rstack_read(""), nullptr, ENOENT);
  CHECK_ERRNO(rstack_read("."), nullptr, EISDIR);
  CHECK_ERRNO(rstack_read("/proc/kcore"), nullptr, EACCES);
  CHECK_ERRNO(rstack_read("/proc/self/mem"), nullptr, EIO);

  ASSERT(n == open_files_count());

  return PASS;
}

static int write_error(void) {
  rstack_t *rs = rstack_new();
  assert(rs);

  CHECK_IF_NO_ERROR(rstack_push_value(rs, 123));

  int n = open_files_count();

  CHECK_ERRNO(rstack_write("", rs), -1, ENOENT);
  CHECK_ERRNO(rstack_write(".", rs), -1, EISDIR);
  CHECK_ERRNO(rstack_write("/etc/hosts", rs), -1, EACCES);
  CHECK_ERRNO(rstack_write("/proc/self/mem", rs), -1, EIO);
  CHECK_ERRNO(rstack_write("/dev/full", rs), -1, ENOSPC);

  ASSERT(n == open_files_count());

  rstack_delete(rs);

  return PASS;
}

// Testujemy reakcję implementacji na niepowodzenie alokacji pamięci.
// Błąd alokacji jest zgłaszany raz. Druga próba powinna się udać.

static unsigned long alloc_fail_read_file(void) {
  unsigned long visited = 0;
  rstack_t *rs;
  result_t result;

  int n = open_files_count();

  errno = 0;
  if ((rs = rstack_read("file_memory_read.in")) != nullptr)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (rs = rstack_read("file_memory_read.in")) != nullptr)
    visited |= V(2, 0);
  else
    return visited |= V(4, 0); // To nie powinno się wykonać.

  errno = 0;
  if ((result = rstack_front(rs)).flag == true)
    visited |= V(1, 1);
  else if (errno != 0 && (result = rstack_front(rs)).flag == true)
    visited |= V(2, 1);
  else
    return visited |= V(4, 1); // To nie powinno się wykonać.

  assert(result.flag == true && result.value == 3);

  rstack_pop(rs);

  errno = 0;
  if ((result = rstack_front(rs)).flag == true)
    visited |= V(1, 2);
  else if (errno != 0 && (result = rstack_front(rs)).flag == true)
    visited |= V(2, 2);
  else
    return visited |= V(4, 2); // To nie powinno się wykonać.

  assert(result.flag == true && result.value == 5);

  rstack_delete(rs);

  assert(n == open_files_count());

  return visited;
}

static unsigned long alloc_fail_write_file(void) {
  unsigned long visited = 0;
  rstack_t *rs, *rt;
  result_t result;

  int n = open_files_count();

  errno = 0;
  if ((rs = rstack_new()) != nullptr)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (rs = rstack_new()) != nullptr)
    visited |= V(2, 0);
  else
    return visited |= V(4, 0); // To nie powinno się wykonać.

  errno = 0;
  if (rstack_push_value(rs, 1) == 0)
    visited |= V(1, 1);
  else if (errno == ENOMEM && rstack_push_value(rs, 1) == 0)
    visited |= V(2, 1);
  else
    return visited |= V(4, 1); // To nie powinno się wykonać.

  errno = 0;
  if ((rt = rstack_new()) != nullptr)
    visited |= V(1, 2);
  else if (errno == ENOMEM && (rt = rstack_new()) != nullptr)
    visited |= V(2, 2);
  else
    return visited |= V(4, 2); // To nie powinno się wykonać.

  errno = 0;
  if (rstack_push_value(rt, 7) == 0)
    visited |= V(1, 3);
  else if (errno == ENOMEM && rstack_push_value(rt, 7) == 0)
    visited |= V(2, 3);
  else
    return visited |= V(4, 3); // To nie powinno się wykonać.

  errno = 0;
  if (rstack_push_rstack(rs, rt) == 0)
    visited |= V(1, 4);
  else if (errno == ENOMEM && rstack_push_rstack(rs, rt) == 0)
    visited |= V(2, 4);
  else
    return visited |= V(4, 4); // To nie powinno się wykonać.

  errno = 0;
  if ((result = rstack_front(rs)).flag == true)
    visited |= V(1, 5);
  else if (errno != 0 && (result = rstack_front(rs)).flag == true)
    visited |= V(2, 5);
  else
    return visited |= V(4, 5); // To nie powinno się wykonać.

  assert(result.flag == true && result.value == 7);

  errno = 0;
  if (rstack_write("file_memory_write.out", rs) == 0)
    visited |= V(1, 6);
  else if (errno != 0 && rstack_write("file_memory_write.out", rs) == 0)
    visited |= V(2, 6);
  else
    return visited |= V(4, 6); // To nie powinno się wykonać.

  rstack_delete(rs);
  rstack_delete(rt);

  assert(n == open_files_count());

  return visited;
}

static int memory_test(unsigned long (* test_function)(void)) {
  memory_test_data_t *mtd = get_memory_test_data();

  unsigned fail = 0, pass = 0;
  mtd->call_total = 0;
  mtd->fail_counter = 1;
  while (fail < 3 && pass < 3) {
    mtd->call_counter = 0;
    mtd->alloc_counter = 0;
    mtd->free_counter = 0;
    mtd->function_name = NULL;
    unsigned long visited_points = test_function();
    if (mtd->alloc_counter != mtd->free_counter ||
        (visited_points & 0444444444444444444444UL) != 0) {
      fprintf(stderr,
              "fail_counter %u, alloc_counter %u, free_counter %u, "
              "function_name %s, visited_point %lo\n",
              mtd->fail_counter, mtd->alloc_counter, mtd->free_counter,
              mtd->function_name, visited_points);
      ++fail;
    }
    if (mtd->function_name == NULL)
      ++pass;
    else
      pass = 0;
    mtd->fail_counter++;
  }

  return mtd->call_total > 0 && fail == 0 ? PASS : FAIL;
}

static int memory_read(void) {
  memory_tests_check();
  return memory_test(alloc_fail_read_file);
}

static int memory_write(void) {
  memory_tests_check();
  return memory_test(alloc_fail_write_file);
}

/** URUCHAMIANIE TESTÓW **/

typedef struct {
  char const *name;
  int (*function)(void);
} test_list_t;

#define TEST(t) {#t, t}

static const test_list_t test_list[] = {
  TEST(zero),
  TEST(one),
  TEST(two),
  TEST(three),
  TEST(four),
  TEST(five),
  TEST(big),
  TEST(cycles),
  TEST(params),
  TEST(parse),
  TEST(read_error),
  TEST(write_error),
  TEST(memory_read),
  TEST(memory_write),
};

static int do_test(int (*function)(void)) {
  int result = function();
  puts("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
  return result;
}

int main(int argc, char *argv[]) {
  if (argc == 2)
    for (size_t i = 0; i < SIZE(test_list); ++i)
      if (strcmp(argv[1], test_list[i].name) == 0)
        return do_test(test_list[i].function);

  fprintf(stderr, "Użycie:\n%s nazwa_testu\n", argv[0]);
  return WRONG_TEST;
}
