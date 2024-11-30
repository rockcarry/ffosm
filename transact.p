<!DOCTYPE html>
<html>
 <head>
  <meta charset="utf-8">
  <title>ffosm 办公物品管理系统 v1.0.0</title>
 </head>
 <body>
  <center><h1>办公物品管理系统</h1></center>
  <hr>
  <h2>%s</h2>
  <form action="transact.cgi">
   <table>
    <tr><td>%s</td><td>%s</td></tr>
    <tr><td>交易用户：</td><td><input type="text" maxlength="64" name="user" value="%s"/></td></tr>
    <tr><td>交易数量：</td><td><input type="text" maxlength="32" name="quantity" value="%s"/></td></tr>
    <tr><td>交易备注：</td><td><input type="text" maxlength="200" name="remarks"/></td></tr>
   </table>
   <br>
   <input type="hidden" name="submit" value="1"/>
   <input type="hidden" name="action" value="%s"/>
   <input type="hidden" name="id" value="%s"/>
   <input type="submit" value="  确  定  "/>&nbsp;&nbsp;
   <input type="button" value="  取  消  " onclick="window.history.back();"/>
  </form>
  <br>
  <hr>
  <p>M2M 事业部</p>
  <br>
 </body>
</html>
