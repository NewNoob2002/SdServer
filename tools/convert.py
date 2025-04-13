#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import gzip
import base64
import re
import shutil
import subprocess
from pathlib import Path
import mimetypes
import hashlib
import time

# 配置部分
INPUT_DIR = "src"  # 输入目录
OUTPUT_DIR = "output"  # 输出目录 (用于临时文件)
TEMP_DIR = "temp_optimized"  # 临时优化目录
OUTPUT_HEADER = "form.h"  # 输出的头文件
CHUNK_SIZE = 16  # 每行显示16个字节

# 确保输出目录存在
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(TEMP_DIR, exist_ok=True)

# 文件类型分类
FILE_TYPES = {
    "html": [".html", ".htm"],
    "css": [".css"],
    "js": [".js"],
    "images": [".png", ".jpg", ".jpeg", ".gif", ".ico", ".webp", ".svg"],
    "fonts": [".woff", ".woff2", ".ttf", ".eot", ".otf"],
    "other": []  # 其他类型
}

# 反向映射文件类型
EXT_TO_TYPE = {}
for file_type, extensions in FILE_TYPES.items():
    for ext in extensions:
        EXT_TO_TYPE[ext] = file_type

def get_file_type(file_path):
    """根据文件扩展名确定文件类型"""
    ext = os.path.splitext(file_path)[1].lower()
    return EXT_TO_TYPE.get(ext, "other")

def get_mime_type(file_path):
    """获取文件的MIME类型"""
    mime_type, _ = mimetypes.guess_type(file_path)
    if mime_type is None:
        # 默认值
        if file_path.endswith('.js'):
            return 'application/javascript'
        elif file_path.endswith('.css'):
            return 'text/css'
        elif file_path.endswith('.woff'):
            return 'font/woff'
        elif file_path.endswith('.woff2'):
            return 'font/woff2'
        elif file_path.endswith('.ttf'):
            return 'font/ttf'
        elif file_path.endswith('.eot'):
            return 'application/vnd.ms-fontobject'
        elif file_path.endswith('.otf'):
            return 'font/otf'
        else:
            return 'application/octet-stream'
    return mime_type

def optimize_image(input_path, output_path):
    """尝试优化图像文件"""
    ext = os.path.splitext(input_path)[1].lower()
    
    # 复制原始文件到临时目录
    shutil.copy2(input_path, output_path)
    
    # 根据图像类型尝试不同的优化
    try:
        if ext in ['.png']:
            # 尝试使用pngquant压缩PNG
            try:
                subprocess.run(['pngquant', '--force', '--output', output_path, input_path], 
                               check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                print(f"  已用pngquant优化: {input_path}")
            except (subprocess.SubprocessError, FileNotFoundError):
                # 如果pngquant不可用，则尝试使用optipng
                try:
                    subprocess.run(['optipng', '-o5', '-quiet', output_path], 
                                  check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    print(f"  已用optipng优化: {input_path}")
                except (subprocess.SubprocessError, FileNotFoundError):
                    pass
                
        elif ext in ['.jpg', '.jpeg']:
            # 尝试使用jpegtran优化JPEG
            try:
                subprocess.run(['jpegtran', '-optimize', '-progressive', '-copy', 'none', 
                                '-outfile', output_path, input_path], 
                               check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                print(f"  已用jpegtran优化: {input_path}")
            except (subprocess.SubprocessError, FileNotFoundError):
                pass
                
        elif ext in ['.gif']:
            # 尝试使用gifsicle优化GIF
            try:
                subprocess.run(['gifsicle', '-O3', '-o', output_path, input_path], 
                               check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                print(f"  已用gifsicle优化: {input_path}")
            except (subprocess.SubprocessError, FileNotFoundError):
                pass
                
        elif ext in ['.webp']:
            # 对于WEBP，可能已经是优化的，但如果有cwebp可以尝试重新编码
            try:
                subprocess.run(['cwebp', '-q', '80', input_path, '-o', output_path], 
                               check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                print(f"  已用cwebp优化: {input_path}")
            except (subprocess.SubprocessError, FileNotFoundError):
                pass
                
    except Exception as e:
        print(f"  优化图像时出错: {input_path} - {str(e)}")
        # 确保输出文件存在，如果发生错误则使用原始文件
        if not os.path.exists(output_path) or os.path.getsize(output_path) == 0:
            shutil.copy2(input_path, output_path)

def minify_js(content):
    """简单的JavaScript压缩"""
    # 移除注释
    content = re.sub(r'/\*[\s\S]*?\*/', '', content)
    content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
    # 移除多余空白
    content = re.sub(r'\s+', ' ', content)
    # 移除行首行尾空白
    content = re.sub(r'^\s+|\s+$', '', content, flags=re.MULTILINE)
    return content

def minify_css(content):
    """简单的CSS压缩"""
    # 移除注释
    content = re.sub(r'/\*[\s\S]*?\*/', '', content)
    # 移除多余空白
    content = re.sub(r'\s+', ' ', content)
    # 移除冒号后的空格
    content = re.sub(r':\s+', ':', content)
    # 移除分号后的空格
    content = re.sub(r';\s+', ';', content)
    # 移除逗号后的空格
    content = re.sub(r',\s+', ',', content)
    # 移除行首行尾空白
    content = re.sub(r'^\s+|\s+$', '', content, flags=re.MULTILINE)
    # 移除最后一个分号
    content = re.sub(r';}', '}', content)
    return content

def minify_html(content):
    """简单的HTML压缩"""
    # 移除注释
    content = re.sub(r'<!--[\s\S]*?-->', '', content)
    # 移除多余空白
    content = re.sub(r'\s+', ' ', content)
    # 移除标签间的空格
    content = re.sub(r'>\s+<', '><', content)
    # 移除行首行尾空白
    content = re.sub(r'^\s+|\s+$', '', content, flags=re.MULTILINE)
    return content

def compress_file(file_path):
    """压缩文件并返回压缩后的内容"""
    file_type = get_file_type(file_path)
    
    # 创建临时优化文件路径
    file_name = os.path.basename(file_path)
    temp_path = os.path.join(TEMP_DIR, file_name)
    
    # 对图像类型进行特殊优化
    if file_type == "images":
        optimize_image(file_path, temp_path)
        optimized_path = temp_path
    else:
        # 对于非图像文件，直接读取
        with open(file_path, 'rb') as f:
            content = f.read()
            
        # 对文本文件进行优化处理
        if file_type in ["html", "css", "js"]:
            try:
                text_content = content.decode('utf-8')
                
                # 根据文件类型应用不同的压缩
                if file_type == "js":
                    text_content = minify_js(text_content)
                elif file_type == "css":
                    text_content = minify_css(text_content)
                elif file_type == "html":
                    text_content = minify_html(text_content)
                
                # 保存优化后的内容
                content = text_content.encode('utf-8')
                with open(temp_path, 'wb') as f:
                    f.write(content)
                optimized_path = temp_path
                
            except UnicodeDecodeError:
                # 如果无法解码为UTF-8，使用原始文件
                optimized_path = file_path
        else:
            # 其他类型文件直接使用
            optimized_path = file_path
    
    # 读取优化后的文件
    with open(optimized_path, 'rb') as f:
        content = f.read()
    
    # GZIP压缩
    compressed = gzip.compress(content, compresslevel=9)
    
    # 如果压缩后大小大于原始大小，则使用原始内容
    if len(compressed) >= len(content):
        return content, False
    
    return compressed, True

def to_c_array(content, var_name, use_progmem=True):
    """将二进制内容转换为C数组，使用指定格式"""
    result = []
    
    # 数组声明（添加static关键字和PROGMEM属性）
    progmem_attr = " PROGMEM" if use_progmem else ""
    result.append(f"static const uint8_t {var_name}[]{progmem_attr} = {{")
    
    # 按每行16个字节格式化数组内容
    for i in range(0, len(content), CHUNK_SIZE):
        line_bytes = content[i:i+CHUNK_SIZE]
        hex_values = [f"0x{b:02X}" for b in line_bytes]
        
        # 最后一行不添加逗号
        if i + CHUNK_SIZE >= len(content):
            result.append("  " + ", ".join(hex_values))
        else:
            result.append("  " + ", ".join(hex_values) + ",")
    
    # 数组结束和长度声明
    result.append("};")
    result.append(f"static const unsigned int {var_name}_len = {len(content)};")
    
    return "\n".join(result)

def sanitize_var_name(name):
    """将文件路径转换为合法的C变量名"""
    # 替换非字母数字字符为下划线
    var_name = re.sub(r'[^a-zA-Z0-9]', '_', name)
    # 确保变量名不以数字开头
    if var_name[0].isdigit():
        var_name = 'f_' + var_name
    # 避免变量名过长
    if len(var_name) > 40:
        # 使用MD5哈希创建唯一且较短的名称
        hash_part = hashlib.md5(name.encode()).hexdigest()[:8]
        var_name = f"{var_name[:30]}_{hash_part}"
    return var_name

def process_files():
    """处理所有文件并生成头文件"""
    file_entries = []
    files_by_type = {file_type: [] for file_type in FILE_TYPES.keys()}
    
    start_time = time.time()
    total_original_size = 0
    total_compressed_size = 0
    
    # 打开头文件，准备写入内容
    with open(OUTPUT_HEADER, 'w', encoding='utf-8') as f_header:
        # 写入文件头部
        f_header.write("""
// AUTO-GENERATED RESOURCES START

// 文件结构定义
struct file_entry {
  const char* path;     // 文件路径
  const uint8_t* data;  // 文件数据
  const unsigned int size; // 文件大小
  const char* mime_type;   // MIME类型
  const bool compressed;   // 是否已压缩
};

""")
        
        # 收集并处理所有文件
        for root, _, files in os.walk(INPUT_DIR):
            for file in files:
                input_path = os.path.join(root, file)
                rel_path = os.path.relpath(input_path, INPUT_DIR)
                file_type = get_file_type(input_path)
                
                # 获取原始文件大小
                original_size = os.path.getsize(input_path)
                total_original_size += original_size
                
                # 压缩文件
                compressed_content, is_compressed = compress_file(input_path)
                total_compressed_size += len(compressed_content)
                
                # 准备文件信息
                var_name = sanitize_var_name(rel_path)
                
                # 将C数组直接写入头文件
                c_array = to_c_array(compressed_content, var_name)
                f_header.write(f"// {rel_path}\n")
                f_header.write(c_array)
                f_header.write("\n\n")
                
                mime_type = get_mime_type(input_path)
                entry = {
                    'path': '/' + rel_path.replace('\\', '/'),
                    'var_name': var_name,
                    'mime_type': mime_type,
                    'compressed': is_compressed,
                    'size': len(compressed_content),
                    'original_size': original_size,
                    'saved': original_size - len(compressed_content)
                }
                
                file_entries.append(entry)
                files_by_type[file_type].append(entry)
                
                compression_rate = 100 - (len(compressed_content) / original_size * 100) if original_size > 0 else 0
                print(f"处理: {rel_path} -> {var_name} ({len(compressed_content):,} 字节, 节省 {compression_rate:.1f}%)")
        
        # 为每种文件类型创建数组
        for file_type, entries in files_by_type.items():
            if not entries:
                continue
                
            f_header.write(f"// {file_type.upper()} 文件\n")
            f_header.write(f"#define {file_type.upper()}_FILES_COUNT {len(entries)}\n")
            
            # 创建文件索引结构体数组
            f_header.write(f"\nconst struct file_entry {file_type}_files[{file_type.upper()}_FILES_COUNT] = {{\n")
            for entry in entries:
                f_header.write(f"  {{ \"{entry['path']}\", {entry['var_name']}, {entry['var_name']}_len, \"{entry['mime_type']}\", {'true' if entry['compressed'] else 'false'} }},\n")
            f_header.write("};\n\n")
        
        f_header.write("// AUTO-GENERATED RESOURCES END\n")
    
    # 计算总节省空间和处理时间
    end_time = time.time()
    processing_time = end_time - start_time
    total_saved = total_original_size - total_compressed_size
    compression_ratio = 100 - (total_compressed_size / total_original_size * 100) if total_original_size > 0 else 0
    
    # 打印摘要信息
    print("\n" + "="*60)
    print(f"前端文件转换工具完成")
    print("="*60)
    print(f"生成的头文件: {OUTPUT_HEADER}")
    print(f"处理了 {len(file_entries)} 个文件，按类型分类如下:")
    
    # 按类型显示统计信息
    for file_type, entries in files_by_type.items():
        if entries:
            type_size = sum(entry['size'] for entry in entries)
            type_original = sum(entry['original_size'] for entry in entries)
            type_ratio = 100 - (type_size / type_original * 100) if type_original > 0 else 0
            print(f"  {file_type}: {len(entries):2d} 个文件, 原始: {type_original:,} 字节, 压缩后: {type_size:,} 字节, 节省: {type_ratio:.1f}%")
    
    # 总体统计
    print("-"*60)
    print(f"总原始大小: {total_original_size:,} 字节")
    print(f"总压缩大小: {total_compressed_size:,} 字节")
    print(f"总节省空间: {total_saved:,} 字节 ({compression_ratio:.1f}%)")
    print(f"处理时间: {processing_time:.2f} 秒")
    print("="*60)

if __name__ == "__main__":
    mimetypes.init()
    try:
        process_files()
    except KeyboardInterrupt:
        print("\n处理被用户中断。")
    except Exception as e:
        print(f"\n处理过程中发生错误: {str(e)}")
    finally:
        print("完成！")
