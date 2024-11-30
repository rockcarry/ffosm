<!DOCTYPE html>
<html>
 <head>
  <meta charset="utf-8">
  <style>
   th, td { text-align: center; vertical-align: middle; }
  </style>
  <script>
   window.onload = function() {
    document.getElementById("qt_type").value = "%s";
   }
  </script>
  <title>ffosm 办公物品管理系统 v1.0.0</title>
 </head>
 <body>
  <center><h1>办公物品管理系统</h1></center>
  <hr>
  <h2>库存清单</h2>
  <form action="transact.cgi" method="post">
   <input type="hidden" name="action" value="create"/>
   物品名称：<input type="text" maxlength="64" name="name"/>&nbsp;
   物品单价：<input type="text" maxlength="32" name="price"/>&nbsp;
   物品备注：<input type="text" maxlength="200" name="remarks"/>&nbsp;
   <input type="submit" value="新建">
  </form>
  <form action="transact.cgi" method="post">
   <input type="hidden" name="action" value="delete"/>
   物品编号：<input type="text" maxlength="32" name="id"/>&nbsp;
   管理密码：<input type="password" maxlength="32" name="passwd"/>&nbsp;
   <input type="submit" value="删除">（执行后将删除指定编号物品的库存和交易记录，危险操作请谨慎执行！）
  </form>
  <form action="ffosm.cgi" method="post">
   物品名称：<input type="text" maxlength="64" name="qs_name" value="%s"/>&nbsp;
   物品备注：<input type="text" maxlength="200" name="qs_remarks" value="%s"/>&nbsp;
   <input type="submit" value="查询">
   <table border="1" width=95%%>
    <tr><th>物品编号</th><th>物品名称</th><th>入库总数</th><th>借出总数</th><th>报废总数</th><th>在库数量</th><th>单价</th><th>物品备注</th><th>操作</th></tr>
    %s
   </table>
   <h2>领用清单</h2>
   用户名：  <input type="text" maxlength="32" name="qb_user" value="%s"/>&nbsp;
   物品名称：<input type="text" maxlength="64" name="qb_name" value="%s"/>&nbsp;
   <input type="submit" value="查询">
   <table border="1" width=95%%>
    <tr><th>用户名</th><th>物品名称</th><th>物品编号</th><th>领用数量</th><th>操作</th></tr>
    %s
   </table>
   <h2>交易记录</h2>
   类型：
   <select name="qt_type" id="qt_type">
    <option value="" >全部</option>
    <option value="1">入库</option>
    <option value="2">领用</option>
    <option value="3">归还</option>
    <option value="4">报废</option>
   </select>
   &nbsp;
   用户名：  <input type="text" maxlength="64" name="qt_user" value="%s"/>&nbsp;
   物品名称：<input type="text" maxlength="64" name="qt_name" value="%s"/>&nbsp;
   交易备注：<input type="text" maxlength="200" name="qt_remarks" value="%s"/>&nbsp;
   <input type="submit" value="查询">
   <table border="1" width=95%%>
    <tr><th>流水号</th><th>类型</th><th>用户名</th><th>物品编号</th><th>物品名称</th><th>数量</th><th>时间</th><th>交易备注</th></tr>
    %s
   </table>
  </form>
  <br>
  <hr>
  <p>M2M 事业部</p>
  <br>
 </body>
</html>
