# Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import paddlenlp
import time
from paddlenlp.transformers import BertTokenizer

t = BertTokenizer.from_pretrained("bert-base-chinese")
line = "泰晤士河水绿如蓝，两岸的建筑物涂染着生机勃发的色彩，阳光也绿意葱笼，为一个季节围起了温情的栅栏。只有圣保罗大教堂不为任何季节所动，一如故我地穿一身灰色法衣，傲岸地站在泰晤士河畔，守望着岁月，它沉郁的钟声，只让浪漫的水手和虔诚的拜谒者感动。林徽因和梁思成将从这里开始他们的造访之旅。林徽因是旧地重游，丝风片云都感到亲切，而梁思成，这里的一切都是陌生的。因着这陌生，他才对这座举世闻名的宗教建筑产生了神秘和向往。遵照父亲梁启超的安排，他们蜜月后的旅行主要是考察古建筑圣保"
# 罗大教堂是他们最先瞩目的第一座圣殿。当他们踏上第一个青石台阶的时候，仿佛踏进了一阕古老的乐章。那是竖琴与占筝合奏的一支宏伟而悲怆的交响。圣保罗大教堂是一座比较成熟的文艺复兴建筑。它碟状形高大的弯窿，以及它的两层楹廊，看上去典雅庄重，整个布局完美和谐，在这里，中世纪的建筑语言几乎完全消失，全部造型生动地反映出文艺复兴建筑文化的特质。这座教堂闻名于世，不仅仅因为它是18世纪著名建筑师克里斯托弗-仑的作品，更因为这里埋葬着曾经打败拿破仑的威灵顿公爵和战功赫赫的海军大将纳尔逊的遗骨。在雕刻着圣保罗旧主生平的山墙下，"
# print(line[:233])
pair = "梁思成问林徽因：“你上看这座教堂，有” "
print(len(line))
print(len(pair))
start = time.time()
for i in range(10000):
    t.encode(line, pair)
end = time.time()
# for k, v in encoded_inputs.items():
#     print(k, " :")
#     for i in v:
#         print(i, end=" ")
#     print()
print("cost time: ", end - start, " sec")
