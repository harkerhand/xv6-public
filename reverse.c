#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// 定义链表节点结构，用于存储每一行文本
typedef struct LineNode
{
    char *line;            // 存储一行文本内容
    struct LineNode *next; // 指向下一个节点
} LineNode;

// 从文件中读取所有行并反向构建链表
LineNode *read_lines(FILE *fp)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    LineNode *head = NULL; // 链表头节点

    // 逐行读取文件内容
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // 为每一行文本分配新节点
        LineNode *new_node = malloc(sizeof(LineNode));
        if (!new_node)
        {
            fprintf(stderr, "malloc failed\n");
            free(line);
            return NULL;
        }

        // 复制行内容
        new_node->line = strdup(line);
        if (!new_node->line)
        {
            fprintf(stderr, "malloc failed\n");
            free(new_node);
            free(line);
            return NULL;
        }

        // 将新节点插入链表头部，实现反向存储
        new_node->next = head;
        head = new_node;
    }

    free(line);
    return head;
}

// 将链表中的行按顺序写入文件
void write_reversed(FILE *fp, LineNode *head)
{
    LineNode *cur = head;
    // 遍历链表，输出每一行
    while (cur)
    {
        fputs(cur->line, fp);
        cur = cur->next;
    }
}

// 释放链表占用的内存
void free_lines(LineNode *head)
{
    LineNode *cur;
    while ((cur = head) != NULL)
    {
        head = head->next; // 保存下一个节点
        free(cur->line);   // 释放行内容
        free(cur);         // 释放节点
    }
}

int main(int argc, char *argv[])
{
    int fd_in = STDIN_FILENO;
    int fd_out = STDOUT_FILENO;
    FILE *in_fp = stdin, *out_fp = stdout;
    struct stat st1, st2;

    // 检查参数数量是否合法
    if (argc > 3)
    {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(EXIT_FAILURE);
    }

    // 处理输入文件
    if (argc >= 2)
    {
        // 打开输入文件
        fd_in = open(argv[1], O_RDONLY);
        if (fd_in < 0)
        {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        // 转换为文件流
        in_fp = fdopen(fd_in, "r");
        if (!in_fp)
        {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            close(fd_in);
            exit(EXIT_FAILURE);
        }
    }

    // 处理输出文件
    if (argc == 3)
    {
        // 获取输入文件信息
        if (stat(argv[1], &st1) < 0)
        {
            fprintf(stderr, "reverse: cannot stat %s: %s\n", argv[1], strerror(errno));
            if (fd_in != STDIN_FILENO)
                close(fd_in);
            exit(EXIT_FAILURE);
        }

        // 检查输入输出是否为同一个文件
        if (stat(argv[2], &st2) == 0)
        {
            if (st1.st_ino == st2.st_ino)
            {
                fprintf(stderr, "reverse: input and output file must differ\n");
                if (fd_in != STDIN_FILENO)
                    close(fd_in);
                exit(EXIT_FAILURE);
            }
        }
        else if (errno != ENOENT)
        {
            // 如果stat出错且不是因为文件不存在，则报错
            fprintf(stderr, "reverse: cannot stat %s: %s\n", argv[2], strerror(errno));
            if (fd_in != STDIN_FILENO)
                close(fd_in);
            exit(EXIT_FAILURE);
        }

        // 打开输出文件
        fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0)
        {
            fprintf(stderr, "reverse: cannot open %s: %s\n", argv[2], strerror(errno));
            if (fd_in != STDIN_FILENO)
                close(fd_in);
            exit(EXIT_FAILURE);
        }

        // 转换为文件流
        out_fp = fdopen(fd_out, "w");
        if (!out_fp)
        {
            fprintf(stderr, "reverse: cannot open %s: %s\n", argv[2], strerror(errno));
            if (fd_in != STDIN_FILENO)
                close(fd_in);
            if (fd_out != STDOUT_FILENO)
                close(fd_out);
            exit(EXIT_FAILURE);
        }
    }

    // 读取所有行
    LineNode *lines = read_lines(in_fp);
    if (!lines && ferror(in_fp))
    {
        fprintf(stderr, "malloc failed\n");
        if (fd_in != STDIN_FILENO)
            close(fd_in);
        if (fd_out != STDOUT_FILENO)
            close(fd_out);
        exit(EXIT_FAILURE);
    }

    // 按反序写出所有行
    write_reversed(out_fp, lines);
    free_lines(lines);

    // 关闭文件
    if (in_fp && in_fp != stdin)
        fclose(in_fp);
    if (out_fp && out_fp != stdout)
        fclose(out_fp);

    exit(EXIT_SUCCESS);
}
