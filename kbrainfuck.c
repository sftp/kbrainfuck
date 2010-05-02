#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

MODULE_DESCRIPTION("Brainfuck interpreter for linux kernel");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sftp");

#define CODE_LEN 1024
#define INPUT_LEN 1024
#define OUTPUT_LEN 1024

static char code[CODE_LEN];
static char input[INPUT_LEN];
static char output[OUTPUT_LEN];

static struct proc_dir_entry *dir_brainfuck, *file_code,
	*file_input, *file_output;

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
	const char *buff, size_t count, loff_t * off)
{
	int len;
	if (count > CODE_LEN)
		len = CODE_LEN;
	else
		len = count;

	copy_from_user(code, buff, len);
	code[len] = '\0';
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
	const char *buff, size_t count, loff_t * off)
{
	int len;
	if (count > CODE_LEN)
		len = CODE_LEN;
	else
		len = count;

	copy_from_user(input, buff, len);
	code[len] = '\0';
	return len;
}

static int output_show(struct seq_file *m, void *v)
{
	seq_printf(m, output);
	return 0;
}

static int output_open(struct inode *inode, struct file *file)
{
	return single_open(file, output_show, NULL);
}

static struct file_operations file_code_ops = {
	.open    = code_open,
	.read    = seq_read,
	.write   = code_write,
	.owner   = THIS_MODULE,
};

static struct file_operations file_input_ops = {
	.open    = input_open,
	.read    = seq_read,
	.write   = input_write,
	.owner   = THIS_MODULE,
};

static struct file_operations file_output_ops = {
	.open    = output_open,
	.read    = seq_read,
	.owner   = THIS_MODULE,
};

static int __init kbrainfuck_init(void)
{
	/* Create /proc/brainfuck directory */
	dir_brainfuck = proc_mkdir("brainfuck", NULL);
	if (dir_brainfuck == 0) {
		printk("kbrainfuck: "
			"Unable to create /proc/brainfuck directory\n");
		return -ENOMEM;
	} else {
		printk("kbrainfuck: "
			"/proc/brainfuck directory created successfully\n");
	}
	/* dir_brainfuck->proc_fops = &dir_brainfuck_ops; */


	/* Create /proc/brainfuck/code entry */
	file_code = create_proc_entry("code", 0666, dir_brainfuck);
	if (file_code == 0) {
		printk("kbrainfuck: "
			"Unable to create /proc/brainfuck/code entry\n");
		return -ENOMEM;
	} else {
		printk("kbrainfuck: "
			"/proc/brainfuck/code entry created successfully\n");
	}
	file_code->proc_fops = &file_code_ops;

	/* Create /proc/brainfuck/input entry */
	file_input = create_proc_entry("input", 0666, dir_brainfuck);
	if (file_input == 0) {
		printk("kbrainfuck: "
			"Unable to create /proc/brainfuck/input entry\n");
		return -ENOMEM;
	} else {
		printk("kbrainfuck: "
			"/proc/brainfuck/input entry created successfully\n");
	}
	file_input->proc_fops = &file_input_ops;

	/* Create /proc/brainfuck/output entry */
	file_output = create_proc_entry("output", NULL, dir_brainfuck);
	if (file_output == 0) {
		printk("kbrainfuck: "
			"Unable to create /proc/brainfuck/output entry\n");
		return -ENOMEM;
	} else {
		printk("kbrainfuck: "
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
