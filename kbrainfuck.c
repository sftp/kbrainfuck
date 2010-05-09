#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

MODULE_DESCRIPTION("Brainfuck interpreter for linux kernel");
MODULE_VERSION("0.2");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sftp");

#define CODE_LEN 1024
#define INPUT_LEN 1024
#define OUTPUT_LEN 1024

#define AREA_SIZE 1024
#define STACK_SIZE 48

#define MAX_OPS 2048

unsigned int stack[STACK_SIZE];
unsigned int stack_pos;

unsigned int code_pos;
unsigned int input_pos;
unsigned int output_pos;
unsigned int area_pos;

unsigned int ops;

static unsigned char code[CODE_LEN];
static unsigned char input[INPUT_LEN];
static unsigned char output[OUTPUT_LEN];

static unsigned char area[AREA_SIZE];

static unsigned char recalc;

static struct proc_dir_entry *dir_brainfuck, *file_code,
	*file_input, *file_output;

int find_brace(int *code_pos)
{
	int braces = 0;

	for ((*code_pos)++; code[*code_pos]; (*code_pos)++) {
		if (code[*code_pos] == ']' && braces == 0)
			return 0;
		else if (code[*code_pos] == '[')
			braces++;
		else if (code[*code_pos] == ']')
			braces--;
	}
	return 1;
}

int push(int x)
{
	stack[stack_pos] = x;
	return ++stack_pos;
}

int pop(void)
{
	return --stack_pos;
}

int read_head(void)
{
	return stack[stack_pos - 1];
}

int brf(void)
{
	while (code[code_pos] && ops) {
		switch (code[code_pos]) {
		case '+':
			area[area_pos]++;
			break;
		case '-':
			area[area_pos]--;
			break;
		case '>':
			if (area_pos < AREA_SIZE - 1)
				area_pos++;
			else
				area_pos = 0;
			break;
		case '<':
			if (area_pos > 0)
				area_pos--;
			else
				area_pos = AREA_SIZE - 1;
			break;
		case '.':
			output[output_pos] = area[area_pos];
			output_pos++;
			break;
		case ',':
			if (input[input_pos] != '\0') {
				area[area_pos] = input[input_pos];
				input_pos++;
			}
			break;
		case '[':
			if (area[area_pos])
				push(code_pos);
			else if (find_brace(&code_pos))
				return 1;
			break;
		case ']':
			if (area[area_pos])
				code_pos = read_head();
			else
				pop();
			break;
		}
		code_pos++;
		ops--;
	}

	return 0;
}

static int code_show(struct seq_file *m, void *v)
{
	seq_printf(m, code);
	return 0;
}

static int code_open(struct inode *inode, struct file *file)
{
	return single_open(file, code_show, NULL);
}

static ssize_t code_write(struct file *file,
	const char *buff, size_t count, loff_t *off)
{
	int len;
	if (count > CODE_LEN)
		len = CODE_LEN;
	else
		len = count;

	copy_from_user(code, buff, len);
	code[len] = '\0';
	code_pos = 0;
	recalc = 1;
	return len;
}

static int input_show(struct seq_file *m, void *v)
{
	seq_printf(m, input);
	return 0;
}

static int input_open(struct inode *inode, struct file *file)
{
	return single_open(file, input_show, NULL);
}

static ssize_t input_write(struct file *file,
	const char *buff, size_t count, loff_t *off)
{
	int len;
	if (count > CODE_LEN)
		len = CODE_LEN;
	else
		len = count;

	copy_from_user(input, buff, len);
	input[len] = '\0';
	input_pos = 0;
	recalc = 1;
	return len;
}

static int output_show(struct seq_file *m, void *v)
{
	unsigned int i;
	if (recalc) {
		for (i = 0; i < OUTPUT_LEN; i++)
			output[i] = '\0';
		for (i = 0; i < AREA_SIZE; i++)
			area[i] = '\0';

		output_pos = 0;
		input_pos = 0;
		stack_pos = 0;
		code_pos = 0;
		area_pos = 0;
		ops = MAX_OPS;
		brf();
		recalc = 0;
	}
	seq_printf(m, output);
	return 0;
}

static int output_open(struct inode *inode, struct file *file)
{
	return single_open(file, output_show, NULL);
}

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

static int __init kbrainfuck_init(void)
{
	dir_brainfuck = proc_mkdir("brainfuck", NULL);
	if (dir_brainfuck == 0) {
		printk(KERN_WARNING "kbrainfuck: "
			"Unable to create /proc/brainfuck directory\n");
		return -ENOMEM;
	} else {
		printk(KERN_INFO "kbrainfuck: "
			"/proc/brainfuck directory created successfully\n");
	}

	file_code = create_proc_entry("code", 0666, dir_brainfuck);
	if (file_code == 0) {
		printk(KERN_WARNING "kbrainfuck: "
			"Unable to create /proc/brainfuck/code entry\n");
		return -ENOMEM;
	} else {
		printk(KERN_INFO "kbrainfuck: "
			"/proc/brainfuck/code entry created successfully\n");
	}
	file_code->proc_fops = &file_code_ops;

	file_input = create_proc_entry("input", 0666, dir_brainfuck);
	if (file_input == 0) {
		printk(KERN_WARNING "kbrainfuck: "
			"Unable to create /proc/brainfuck/input entry\n");
		return -ENOMEM;
	} else {
		printk(KERN_INFO "kbrainfuck: "
			"/proc/brainfuck/input entry created successfully\n");
	}
	file_input->proc_fops = &file_input_ops;

	file_output = create_proc_entry("output", NULL, dir_brainfuck);
	if (file_output == 0) {
		printk(KERN_WARNING "kbrainfuck: "
			"Unable to create /proc/brainfuck/output entry\n");
		return -ENOMEM;
	} else {
		printk(KERN_INFO "kbrainfuck: "
			"/proc/brainfuck/output entry created successfully\n");
	}
	file_output->proc_fops = &file_output_ops;

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
