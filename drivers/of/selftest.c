#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define pr_fmt(fmt) "### dt-test ### " fmt

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/of.h>
#if defined(MY_ABC_HERE)
#include <linux/of_irq.h>
#endif  
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/device.h>

#if defined(MY_ABC_HERE)
static struct selftest_results {
	int passed;
	int failed;
} selftest_results;

#define selftest(result, fmt, ...) { \
	if (!(result)) { \
		selftest_results.failed++; \
		pr_err("FAIL %s():%i " fmt, __func__, __LINE__, ##__VA_ARGS__); \
	} else { \
		selftest_results.passed++; \
		pr_debug("pass %s():%i\n", __func__, __LINE__); \
	} \
}
#else  
static bool selftest_passed = true;

#define selftest(result, fmt, ...) { \
	if (!(result)) { \
		pr_err("FAIL %s:%i " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
		selftest_passed = false; \
	} else { \
		pr_info("pass %s:%i\n", __FILE__, __LINE__); \
	} \
}
#endif  

static void __init of_selftest_parse_phandle_with_args(void)
{
	struct device_node *np;
	struct of_phandle_args args;
	int i, rc;

	np = of_find_node_by_path("/testcase-data/phandle-tests/consumer-a");
	if (!np) {
		pr_err("missing testcase data\n");
		return;
	}

	rc = of_count_phandle_with_args(np, "phandle-list", "#phandle-cells");
	selftest(rc == 7, "of_count_phandle_with_args() returned %i, expected 7\n", rc);

	for (i = 0; i < 8; i++) {
		bool passed = true;
		rc = of_parse_phandle_with_args(np, "phandle-list",
						"#phandle-cells", i, &args);

		switch (i) {
		case 0:
			passed &= !rc;
			passed &= (args.args_count == 1);
			passed &= (args.args[0] == (i + 1));
			break;
		case 1:
			passed &= !rc;
			passed &= (args.args_count == 2);
			passed &= (args.args[0] == (i + 1));
			passed &= (args.args[1] == 0);
			break;
		case 2:
			passed &= (rc == -ENOENT);
			break;
		case 3:
			passed &= !rc;
			passed &= (args.args_count == 3);
			passed &= (args.args[0] == (i + 1));
			passed &= (args.args[1] == 4);
			passed &= (args.args[2] == 3);
			break;
		case 4:
			passed &= !rc;
			passed &= (args.args_count == 2);
			passed &= (args.args[0] == (i + 1));
			passed &= (args.args[1] == 100);
			break;
		case 5:
			passed &= !rc;
			passed &= (args.args_count == 0);
			break;
		case 6:
			passed &= !rc;
			passed &= (args.args_count == 1);
			passed &= (args.args[0] == (i + 1));
			break;
		case 7:
			passed &= (rc == -ENOENT);
			break;
		default:
			passed = false;
		}

		selftest(passed, "index %i - data error on node %s rc=%i\n",
			 i, args.np->full_name, rc);
	}

	rc = of_parse_phandle_with_args(np, "phandle-list-missing",
					"#phandle-cells", 0, &args);
	selftest(rc == -ENOENT, "expected:%i got:%i\n", -ENOENT, rc);
	rc = of_count_phandle_with_args(np, "phandle-list-missing",
					"#phandle-cells");
	selftest(rc == -ENOENT, "expected:%i got:%i\n", -ENOENT, rc);

	rc = of_parse_phandle_with_args(np, "phandle-list",
					"#phandle-cells-missing", 0, &args);
	selftest(rc == -EINVAL, "expected:%i got:%i\n", -EINVAL, rc);
	rc = of_count_phandle_with_args(np, "phandle-list",
					"#phandle-cells-missing");
	selftest(rc == -EINVAL, "expected:%i got:%i\n", -EINVAL, rc);

	rc = of_parse_phandle_with_args(np, "phandle-list-bad-phandle",
					"#phandle-cells", 0, &args);
	selftest(rc == -EINVAL, "expected:%i got:%i\n", -EINVAL, rc);
	rc = of_count_phandle_with_args(np, "phandle-list-bad-phandle",
					"#phandle-cells");
	selftest(rc == -EINVAL, "expected:%i got:%i\n", -EINVAL, rc);

	rc = of_parse_phandle_with_args(np, "phandle-list-bad-args",
					"#phandle-cells", 1, &args);
	selftest(rc == -EINVAL, "expected:%i got:%i\n", -EINVAL, rc);
	rc = of_count_phandle_with_args(np, "phandle-list-bad-args",
					"#phandle-cells");
	selftest(rc == -EINVAL, "expected:%i got:%i\n", -EINVAL, rc);
}

static void __init of_selftest_property_string(void)
{
	const char *strings[4];
	struct device_node *np;
	int rc;

	pr_info("start\n");
	np = of_find_node_by_path("/testcase-data/phandle-tests/consumer-a");
	if (!np) {
		pr_err("No testcase data in device tree\n");
		return;
	}

	rc = of_property_match_string(np, "phandle-list-names", "first");
	selftest(rc == 0, "first expected:0 got:%i\n", rc);
	rc = of_property_match_string(np, "phandle-list-names", "second");
	selftest(rc == 1, "second expected:0 got:%i\n", rc);
	rc = of_property_match_string(np, "phandle-list-names", "third");
	selftest(rc == 2, "third expected:0 got:%i\n", rc);
	rc = of_property_match_string(np, "phandle-list-names", "fourth");
	selftest(rc == -ENODATA, "unmatched string; rc=%i\n", rc);
	rc = of_property_match_string(np, "missing-property", "blah");
	selftest(rc == -EINVAL, "missing property; rc=%i\n", rc);
	rc = of_property_match_string(np, "empty-property", "blah");
	selftest(rc == -ENODATA, "empty property; rc=%i\n", rc);
	rc = of_property_match_string(np, "unterminated-string", "blah");
	selftest(rc == -EILSEQ, "unterminated string; rc=%i\n", rc);

	rc = of_property_count_strings(np, "string-property");
	selftest(rc == 1, "Incorrect string count; rc=%i\n", rc);
	rc = of_property_count_strings(np, "phandle-list-names");
	selftest(rc == 3, "Incorrect string count; rc=%i\n", rc);
	rc = of_property_count_strings(np, "unterminated-string");
	selftest(rc == -EILSEQ, "unterminated string; rc=%i\n", rc);
	rc = of_property_count_strings(np, "unterminated-string-list");
	selftest(rc == -EILSEQ, "unterminated string array; rc=%i\n", rc);

	rc = of_property_read_string_index(np, "string-property", 0, strings);
	selftest(rc == 0 && !strcmp(strings[0], "foobar"), "of_property_read_string_index() failure; rc=%i\n", rc);
	strings[0] = NULL;
	rc = of_property_read_string_index(np, "string-property", 1, strings);
	selftest(rc == -ENODATA && strings[0] == NULL, "of_property_read_string_index() failure; rc=%i\n", rc);
	rc = of_property_read_string_index(np, "phandle-list-names", 0, strings);
	selftest(rc == 0 && !strcmp(strings[0], "first"), "of_property_read_string_index() failure; rc=%i\n", rc);
	rc = of_property_read_string_index(np, "phandle-list-names", 1, strings);
	selftest(rc == 0 && !strcmp(strings[0], "second"), "of_property_read_string_index() failure; rc=%i\n", rc);
	rc = of_property_read_string_index(np, "phandle-list-names", 2, strings);
	selftest(rc == 0 && !strcmp(strings[0], "third"), "of_property_read_string_index() failure; rc=%i\n", rc);
	strings[0] = NULL;
	rc = of_property_read_string_index(np, "phandle-list-names", 3, strings);
	selftest(rc == -ENODATA && strings[0] == NULL, "of_property_read_string_index() failure; rc=%i\n", rc);
	strings[0] = NULL;
	rc = of_property_read_string_index(np, "unterminated-string", 0, strings);
	selftest(rc == -EILSEQ && strings[0] == NULL, "of_property_read_string_index() failure; rc=%i\n", rc);
	rc = of_property_read_string_index(np, "unterminated-string-list", 0, strings);
	selftest(rc == 0 && !strcmp(strings[0], "first"), "of_property_read_string_index() failure; rc=%i\n", rc);
	strings[0] = NULL;
	rc = of_property_read_string_index(np, "unterminated-string-list", 2, strings);  
	selftest(rc == -EILSEQ && strings[0] == NULL, "of_property_read_string_index() failure; rc=%i\n", rc);
	strings[1] = NULL;

	rc = of_property_read_string_array(np, "string-property", strings, 4);
	selftest(rc == 1, "Incorrect string count; rc=%i\n", rc);
	rc = of_property_read_string_array(np, "phandle-list-names", strings, 4);
	selftest(rc == 3, "Incorrect string count; rc=%i\n", rc);
	rc = of_property_read_string_array(np, "unterminated-string", strings, 4);
	selftest(rc == -EILSEQ, "unterminated string; rc=%i\n", rc);
	 
	rc = of_property_read_string_array(np, "unterminated-string-list", strings, 4);
	selftest(rc == -EILSEQ, "unterminated string array; rc=%i\n", rc);
	 
	strings[2] = NULL;
	rc = of_property_read_string_array(np, "unterminated-string-list", strings, 2);
	selftest(rc == 2 && strings[2] == NULL, "of_property_read_string_array() failure; rc=%i\n", rc);
	strings[1] = NULL;
	rc = of_property_read_string_array(np, "phandle-list-names", strings, 1);
	selftest(rc == 1 && strings[1] == NULL, "Overwrote end of string array; rc=%i, str='%s'\n", rc, strings[1]);
}

#if defined(MY_ABC_HERE)
static void __init of_selftest_parse_interrupts(void)
{
	struct device_node *np;
	struct of_phandle_args args;
	int i, rc;

	np = of_find_node_by_path("/testcase-data/interrupts/interrupts0");
	if (!np) {
		pr_err("missing testcase data\n");
		return;
	}

	for (i = 0; i < 4; i++) {
		bool passed = true;
		args.args_count = 0;
		rc = of_irq_parse_one(np, i, &args);

		passed &= !rc;
		passed &= (args.args_count == 1);
		passed &= (args.args[0] == (i + 1));

		selftest(passed, "index %i - data error on node %s rc=%i\n",
			 i, args.np->full_name, rc);
	}
	of_node_put(np);

	np = of_find_node_by_path("/testcase-data/interrupts/interrupts1");
	if (!np) {
		pr_err("missing testcase data\n");
		return;
	}

	for (i = 0; i < 4; i++) {
		bool passed = true;
		args.args_count = 0;
		rc = of_irq_parse_one(np, i, &args);

		switch (i) {
		case 0:
			passed &= !rc;
			passed &= (args.args_count == 1);
			passed &= (args.args[0] == 9);
			break;
		case 1:
			passed &= !rc;
			passed &= (args.args_count == 3);
			passed &= (args.args[0] == 10);
			passed &= (args.args[1] == 11);
			passed &= (args.args[2] == 12);
			break;
		case 2:
			passed &= !rc;
			passed &= (args.args_count == 2);
			passed &= (args.args[0] == 13);
			passed &= (args.args[1] == 14);
			break;
		case 3:
			passed &= !rc;
			passed &= (args.args_count == 2);
			passed &= (args.args[0] == 15);
			passed &= (args.args[1] == 16);
			break;
		default:
			passed = false;
		}
		selftest(passed, "index %i - data error on node %s rc=%i\n",
			 i, args.np->full_name, rc);
	}
	of_node_put(np);
}

static void __init of_selftest_parse_interrupts_extended(void)
{
	struct device_node *np;
	struct of_phandle_args args;
	int i, rc;

	np = of_find_node_by_path("/testcase-data/interrupts/interrupts-extended0");
	if (!np) {
		pr_err("missing testcase data\n");
		return;
	}

	for (i = 0; i < 7; i++) {
		bool passed = true;
		rc = of_irq_parse_one(np, i, &args);

		switch (i) {
		case 0:
			passed &= !rc;
			passed &= (args.args_count == 1);
			passed &= (args.args[0] == 1);
			break;
		case 1:
			passed &= !rc;
			passed &= (args.args_count == 3);
			passed &= (args.args[0] == 2);
			passed &= (args.args[1] == 3);
			passed &= (args.args[2] == 4);
			break;
		case 2:
			passed &= !rc;
			passed &= (args.args_count == 2);
			passed &= (args.args[0] == 5);
			passed &= (args.args[1] == 6);
			break;
		case 3:
			passed &= !rc;
			passed &= (args.args_count == 1);
			passed &= (args.args[0] == 9);
			break;
		case 4:
			passed &= !rc;
			passed &= (args.args_count == 3);
			passed &= (args.args[0] == 10);
			passed &= (args.args[1] == 11);
			passed &= (args.args[2] == 12);
			break;
		case 5:
			passed &= !rc;
			passed &= (args.args_count == 2);
			passed &= (args.args[0] == 13);
			passed &= (args.args[1] == 14);
			break;
		case 6:
			passed &= !rc;
			passed &= (args.args_count == 1);
			passed &= (args.args[0] == 15);
			break;
		default:
			passed = false;
		}

		selftest(passed, "index %i - data error on node %s rc=%i\n",
			 i, args.np->full_name, rc);
	}
	of_node_put(np);
}
#endif  

static int __init of_selftest(void)
{
	struct device_node *np;

	np = of_find_node_by_path("/testcase-data/phandle-tests/consumer-a");
	if (!np) {
		pr_info("No testcase data in device tree; not running tests\n");
		return 0;
	}
	of_node_put(np);

	pr_info("start of selftest - you will see error messages\n");
	of_selftest_parse_phandle_with_args();
	of_selftest_property_string();
#if defined(MY_ABC_HERE)
	of_selftest_parse_interrupts();
	of_selftest_parse_interrupts_extended();
	pr_info("end of selftest - %i passed, %i failed\n",
		selftest_results.passed, selftest_results.failed);
#else  
	pr_info("end of selftest - %s\n", selftest_passed ? "PASS" : "FAIL");
#endif  
	return 0;
}
late_initcall(of_selftest);
