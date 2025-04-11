const form = document.getElementById('uploadForm');
const fileInput = document.getElementById('firmwareFile');
const submitBtn = document.getElementById('submit-btn');
const progressBar = document.querySelector('.bar');
const statusText = document.getElementById('status');

// 验证文件类型
function validateFile(file) {
    const allowedTypes = ['application/octet-stream', 'application/x-binary'];
    const maxSize = 4 * 1024 * 1024; // 4MB
    
    if (!file.name.endsWith('.bin')) {
        alert('错误：请选择.bin格式的固件文件');
        return false;
    }
    
    if (file.size > maxSize) {
        alert('错误：文件大小不能超过4MB');
        return false;
    }
    
    return true;
}

form.addEventListener('submit', function(e) {
    e.preventDefault();
    
    const file = fileInput.files[0];
    if (!file || !validateFile(file)) return;

    submitBtn.disabled = true;
    statusText.textContent = '上传中...';
    progressBar.style.width = '0%';
    progressBar.style.backgroundColor = '#4CAF50';

    const xhr = new XMLHttpRequest();
    const formData = new FormData(form);

    xhr.upload.addEventListener('progress', function(evt) {
        if (evt.lengthComputable) {
            const percent = Math.round((evt.loaded / evt.total) * 100);
            progressBar.style.width = percent + '%';
        }
    });

    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            submitBtn.disabled = false;
            if (xhr.status === 200) {
                progressBar.style.backgroundColor = '#4CAF50';
                statusText.textContent = '升级成功，设备即将重启...';
                setTimeout(() => {
                    window.location.reload();
                }, 3000);
            } else {
                progressBar.style.backgroundColor = '#f44336';
                statusText.textContent = `升级失败: ${xhr.responseText || '未知错误'}`;
            }
        }
    };

    xhr.open('POST', '/update', true);
    xhr.send(formData);
});

// 文件选择变化时显示文件名
fileInput.addEventListener('change', function() {
    if (this.files.length > 0) {
        statusText.textContent = `已选择文件: ${this.files[0].name}`;
    }
});