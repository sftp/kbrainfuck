#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

MODULE_DESCRIPTION("Brainfuck interpreter for linux kernel");
MODULE_VERSION("0.4");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sftp");

#define kbrf_warn(fmt, args...) printk(KERN_WARNING fmt, ##args)

#define CODE_LEN 1024
#define INPUT_LEN 1024
#define OUTPUT_LEN 1024

#define AREA_SIZE 1024
#define STACK_SIZE 48

#define MAX_OPS 4096

struct kbrf_ctx {
	u32 stack[STACK_SIZE];
	u8 code[CODE_LEN];
	u8 input[INPUT_LEN];
	u8 output[OUTPUT_LEN];
	u8 area[AREA_SIZE];

	u32 stack_pos;
	u32 code_pos;
	u32 input_pos;
	u32 output_pos;
	u32 area_pos;

	u32 ops;
	u8 need_recalc : 1;
};

static struct proc_dir_entry *dir_brainfuck, *file_code,
	*file_input, *file_output;

static struct kbrf_ctx ctx;

u8 find_brace(void)
{
	s32 braces = 0;

	for (ctx.code_pos++; ctx.code[ctx.code_pos]; ctx.code_pos++) {
		if (ctx.code[ctx.code_pos] == ']' && braces == 0)
			return 0;
		else if (ctx.code[ctx.code_pos] == '[')
			braces++;
		else if (ctx.code[ctx.code_pos] == ']')
			braces--;
	}

	return 1;
}

u32 push(u32 x)
{
	ctx.stack[ctx.stack_pos] = x;

	return ++ctx.stack_pos;
}

u32 pop(void)
{
	return --ctx.stack_pos;
}

u32 peek(void)
{
	return ctx.stack[ctx.stack_pos - 1];
}

int brf(void)
{
	while (ctx.code[ctx.code_pos] && ctx.ops) {
		switch (ctx.code[ctx.code_pos]) {
		case '+':
			ctx.area[ctx.area_pos]++;
			break;
		case '-':
			ctx.area[ctx.area_pos]--;
			break;
		case '>':
			if (ctx.area_pos < AREA_SIZE - 1)
				ctx.area_pos++;
			else
				ctx.area_pos = 0;
			break;
		case '<':
			if (ctx.area_pos > 0)
				ctx.area_pos--;
			else
				ctx.area_pos = AREA_SIZE - 1;
			break;
		case '.':
			ctx.output[ctx.output_pos] = ctx.area[ctx.area_pos];
			ctx.output_pos++;
			break;
		case ',':
			if (ctx.input[ctx.input_pos] != '\0') {
				ctx.area[ctx.area_pos] = ctx.input[ctx.input_pos];
				ctx.input_pos++;
			}
			break;
		case '[':
			if (ctx.area[ctx.area_pos])
				push(ctx.code_pos);
			else if (find_brace())
				return 1;
			break;
		case ']':
			if (ctx.area[ctx.area_pos])
				ctx.code_pos = peek();
			else
				pop();
			break;
		}

		ctx.code_pos++;
		ctx.ops--;
	}

	return 0;
}

static int code_show(struct seq_file *m, void *v)
{
	seq_printf(m, ctx.code);

	return 0;
}

static int code_open(struct inode *inode, struct file *file)
{
	return single_open(file, code_show, NULL);
}

static ssize_t code_write(struct file *file,
	const char *buff, size_t count, loff_t *off)
{
	ssize_t len;

	if (count > CODE_LEN)
		len = CODE_LEN;
	else
		len = count;

	if (copy_from_user(ctx.code, buff, len))
		return -1;

	ctx.code[len] = '\0';
	ctx.code_pos = 0;
	ctx.need_recalc = 1;

	return len;
}

static int input_show(struct seq_file *m, void *v)
{
	seq_printf(m, ctx.input);

	return 0;
}

static int input_open(struct inode *inode, struct file *file)
{
	return single_open(file, input_show, NULL);
}

static ssize_t input_write(struct file *file,
	const char *buff, size_t count, loff_t *off)
{
	ssize_t len;

	if (count > CODE_LEN)
		len = CODE_LEN;
	else
		len = count;

	if (copy_from_user(ctx.input, buff, len))
		return -1;

	ctx.input[len] = '\0';
	ctx.input_pos = 0;
	ctx.need_recalc = 1;

	return len;
}

static int output_show(struct seq_file *m, void *v)
{
	u32 i;

	if (ctx.need_recalc) {
		for (i = 0; i < OUTPUT_LEN; i++)
			ctx.output[i] = '\0';
		for (i = 0; i < AREA_SIZE; i++)
			ctx.area[i] = '\0';

		ctx.output_pos = 0;
		ctx.input_pos = 0;
		ctx.stack_pos = 0;
		ctx.code_pos = 0;
		ctx.area_pos = 0;
		ctx.ops = MAX_OPS;
		brf();
		ctx.need_recalc = 0;
	}

	seq_printf(m, ctx.output);

	return 0;
}

static int output_open(struct inode *inode, struct file *file)
{
	return single_open(file, output_show, NULL);
}

static int __init kbrainfuck_init(void)
{
	static const struct file_operations file_code_ops = {
		.open    = code_open,
		.read    = seq_read,
		.write   = code_write,
		.owner   = THIS_MODULE,
	};

	static const struct file_operations file_input_ops = {
		.open    = input_open,
		.read    = seq_read,
		.write   = input_write,
		.owner   = THIS_MODULE,
	};

	static const struct file_operations file_output_ops = {
		.open    = output_open,
		.read    = seq_read,
		.owner   = THIS_MODULE,
	};

	dir_brainfuck = proc_mkdir("brainfuck", NULL);
	if (dir_brainfuck == 0) {
		kbrf_warn("Unable to create /proc/brainfuck directory\n");
		return -ENOMEM;
	}

	file_code = proc_create("code", 0666, dir_brainfuck, &file_code_ops);
	if (file_code == 0) {
		kbrf_warn("Unable to create /proc/brainfuck/code entry\n");
		return -ENOMEM;
	}

	file_input = proc_create("input", 0666, dir_brainfuck, &file_input_ops);
	if (file_input == 0) {
		kbrf_warn("Unable to create /proc/brainfuck/input entry\n");
		return -ENOMEM;
	}

	file_output = proc_create("output", 0444, dir_brainfuck, &file_output_ops);
	if (file_output == 0) {
		kbrf_warn("Unable to create /proc/brainfuck/output entry\n");
		return -ENOMEM;
	}

	return 0;
}

module_init(kbrainfuck_init);

static void __exit kbrainfuck_exit(void)
{
	remove_proc_entry("code", dir_brainfuck);
	remove_proc_entry("input", dir_brainfuck);
	remove_proc_entry("output", dir_brainfuck);
	remove_proc_entry("brainfuck", NULL);
}

module_exit(kbrainfuck_exit);
